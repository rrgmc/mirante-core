// Tests the module:// scheme: which addresses it serves, which pages may load
// them, its headers, and that each module's pages have their own origin. The
// pages come from tests/web/modules/, served as module://test/ and
// module://other/. See docs/web-views.md#module-pages.
#include "web/ModuleSchemeHandler.h"
#include "web/ProfileManager.h"

#include <QApplication>
#include <QLibraryInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>
#include <QWebEnginePage>
#include <QWebEngineProfile>

#include <memory>

using namespace mirante;

namespace {

// Starting Chromium for the first page takes a while.
constexpr int kTimeout = 20000;

const QUrl testPage(QStringLiteral("module://test/index.html"));
const QUrl otherPage(QStringLiteral("module://other/index.html"));

// The value of a JavaScript expression in the page. A promise is awaited.
QVariant evaluate(QWebEnginePage &page, const QString &script)
{
    QVariant result;
    bool done = false;
    page.runJavaScript(script, [&](const QVariant &value) {
        result = value;
        done = true;
    });
    if (!QTest::qWaitFor([&] { return done; }, kTimeout))
        return {};
    return result;
}

// Loads url in a new page of profile, and returns the page once it loaded.
std::unique_ptr<QWebEnginePage> load(QWebEngineProfile *profile, const QUrl &url, bool *ok)
{
    auto page = std::make_unique<QWebEnginePage>(profile);
    QSignalSpy loaded(page.get(), &QWebEnginePage::loadFinished);
    page->load(url);
    *ok = loaded.wait(kTimeout) && loaded.first().first().toBool();
    return page;
}

} // namespace

class TestModuleScheme : public QObject
{
    Q_OBJECT

private slots:
    void resourceFor_data()
    {
        QTest::addColumn<QString>("url");
        QTest::addColumn<QString>("resource");

        QTest::newRow("page") << "module://acme/index.html" << ":/modules/acme/web/index.html";
        QTest::newRow("below") << "module://acme/a/b.js" << ":/modules/acme/web/a/b.js";
        QTest::newRow("query ignored") << "module://acme/index.html?x=1" << ":/modules/acme/web/index.html";
        QTest::newRow("capitals") << "module://ACME/index.html" << ":/modules/acme/web/index.html";
        QTest::newRow("not an id") << "module://acme-x/index.html" << "";
        QTest::newRow("no host") << "module:/index.html" << "";
        QTest::newRow("other scheme") << "qrc://acme/index.html" << "";
        QTest::newRow("empty path") << "module://acme" << "";
        QTest::newRow("root") << "module://acme/" << "";
        QTest::newRow("folder") << "module://acme/a/" << "";
        QTest::newRow("double slash") << "module://acme/a//b.js" << "";
        QTest::newRow("dot dot") << "module://acme/../other/web/x.js" << "";
        QTest::newRow("encoded dot dot") << "module://acme/%2e%2e/%2E%2E/x.js" << "";
        QTest::newRow("encoded slash") << "module://acme/a%2f..%2f..%2fx.js" << "";
        QTest::newRow("dot") << "module://acme/./index.html" << "";
        QTest::newRow("backslash") << "module://acme/a%5c..%5cx.js" << "";
        QTest::newRow("colon") << "module://acme/c:/x.js" << "";
    }

    void resourceFor()
    {
        QFETCH(QString, url);
        QFETCH(QString, resource);
        const std::optional<QString> found = modulepages::resourceFor(QUrl(url));
        QCOMPARE(found.value_or(QString()), resource);
    }

    void contentTypes()
    {
        QCOMPARE(modulepages::contentTypeFor(QStringLiteral("a/index.html")), QByteArray("text/html"));
        QCOMPARE(modulepages::contentTypeFor(QStringLiteral("a/page.js")), QByteArray("text/javascript"));
        QCOMPARE(modulepages::contentTypeFor(QStringLiteral("a/page.CSS")), QByteArray("text/css"));
        QCOMPARE(modulepages::contentTypeFor(QStringLiteral("a/data.json")), QByteArray("application/json"));
        QCOMPARE(modulepages::contentTypeFor(QStringLiteral("a/icon.svg")), QByteArray("image/svg+xml"));
        QCOMPARE(modulepages::contentTypeFor(QStringLiteral("a/photo.jpeg")), QByteArray("image/jpeg"));
        QVERIFY(modulepages::contentTypeFor(QStringLiteral("a/README")).isEmpty());
        QVERIFY(modulepages::contentTypeFor(QStringLiteral("a/run.exe")).isEmpty());
    }

    void allowed()
    {
        const QUrl own(QStringLiteral("module://acme/data.json"));
        QVERIFY(modulepages::allowed(QUrl(), own));
        QVERIFY(modulepages::allowed(QUrl(QStringLiteral("module://acme")), own));
        QVERIFY(!modulepages::allowed(QUrl(QStringLiteral("module://other")), own));
        QVERIFY(!modulepages::allowed(QUrl(QStringLiteral("https://acme")), own));
        QVERIFY(!modulepages::allowed(QUrl(QStringLiteral("dash://panels")), own));
    }

    void headers()
    {
        const QMultiMap<QByteArray, QByteArray> headers = modulepages::responseHeaders();
        const QByteArray policy = headers.value("Content-Security-Policy");
        QVERIFY(policy.startsWith("default-src 'self';"));
        QVERIFY(policy.contains("connect-src 'self'"));
        QVERIFY(policy.contains("object-src 'none'"));
        QCOMPARE(headers.value("X-Content-Type-Options"), QByteArray("nosniff"));
        QCOMPARE(headers.value("Cache-Control"), QByteArray("no-store"));
    }

    void modulePages()
    {
        QVERIFY(modulepages::isModulePage(testPage));
        QVERIFY(!modulepages::isModulePage(QUrl(QStringLiteral("https://example.com/"))));
    }

    // A profile of the list serves the page and its own files under the
    // policy, which stops the inline script.
    void profileLoadsModulePages()
    {
        // The debug WebEngine stops on its own checks on the offscreen
        // platform. See docs/web-views.md#debug-builds.
        if (QLibraryInfo::isDebugBuild())
            QSKIP("The debug WebEngine stops on its own checks here; run this test in a release build.");
        QTemporaryDir root;
        ProfileManager profiles(root.path());
        QWebEngineProfile *work = profiles.profile(QStringLiteral("default"));
        QVERIFY(work->urlSchemeHandler(modulepages::schemeName()));
        // Asking again installs nothing more.
        QCOMPARE(profiles.profile(QStringLiteral("default")), work);

        bool ok = false;
        auto page = load(work, testPage, &ok);
        QVERIFY(ok);
        QCOMPARE(evaluate(*page, QStringLiteral("document.title")).toString(), QStringLiteral("Module test"));
        QCOMPARE(evaluate(*page, QStringLiteral("window.moduleTestLoaded === true")).toBool(), true);
        QCOMPARE(evaluate(*page, QStringLiteral("window.isSecureContext")).toBool(), true);
        QCOMPARE(evaluate(*page, QStringLiteral("location.origin")).toString(), QStringLiteral("module://test"));

        // The page can fetch its own files.
        evaluate(*page, QStringLiteral("fetch('data.json').then(r => r.json())"
                                       ".then(d => { window.fetched = d.greeting; }, e => { window.fetched = String(e); });"
                                       " true"));
        QTRY_COMPARE_WITH_TIMEOUT(evaluate(*page, QStringLiteral("window.fetched")).toString(), QStringLiteral("hello"),
                                  kTimeout);
        page.reset();
    }

    // Two modules' pages keep their storage apart, and one cannot run the
    // other's script.
    void eachModuleHasItsOwnOrigin()
    {
        if (QLibraryInfo::isDebugBuild())
            QSKIP("The debug WebEngine stops on its own checks here; run this test in a release build.");
        QTemporaryDir root;
        ProfileManager profiles(root.path());
        QWebEngineProfile *work = profiles.profile(QStringLiteral("default"));

        bool ok = false;
        auto test = load(work, testPage, &ok);
        QVERIFY(ok);
        evaluate(*test, QStringLiteral("localStorage.setItem('secret', 'test'); true"));

        auto other = load(work, otherPage, &ok);
        QVERIFY(ok);
        QCOMPARE(evaluate(*other, QStringLiteral("location.origin")).toString(), QStringLiteral("module://other"));
        QCOMPARE(evaluate(*other, QStringLiteral("localStorage.getItem('secret') === null")).toBool(), true);
        QCOMPARE(evaluate(*other, QStringLiteral("window.moduleTestLoaded === undefined")).toBool(), true);
        other.reset();
        test.reset();
    }

    // The app profile has no handler, so a module's page does not load there.
    void appProfileRefusesModulePages()
    {
        if (QLibraryInfo::isDebugBuild())
            QSKIP("The debug WebEngine stops on its own checks here; run this test in a release build.");
        QTemporaryDir root;
        ProfileManager profiles(root.path());
        QWebEngineProfile *app = profiles.appProfile();
        QVERIFY(!app->urlSchemeHandler(modulepages::schemeName()));
        bool ok = true;
        auto page = load(app, testPage, &ok);
        QVERIFY(!ok);
        page.reset();
    }
};

// The scheme must be registered before the application is made, so this test
// has its own main().
int main(int argc, char *argv[])
{
    modulepages::registerScheme();
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication app(argc, argv);
    TestModuleScheme test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "tst_module_scheme.moc"
