#include "web/ModuleSchemeHandler.h"

#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QUrl>
#include <QWebEngineUrlRequestJob>
#include <QWebEngineUrlScheme>

namespace mirante {

namespace modulepages {

namespace {

bool registered = false;

// The rule of module ids in <mirante/ModuleIds.h>, which this library does
// not link.
bool isModuleId(const QString &host)
{
    static const QRegularExpression id(QStringLiteral("^[a-z0-9]+$"));
    return id.match(host).hasMatch();
}

} // namespace

QByteArray schemeName() { return QByteArrayLiteral("module"); }

void registerScheme()
{
    QWebEngineUrlScheme scheme(schemeName());
    scheme.setSyntax(QWebEngineUrlScheme::Syntax::Host);
    // A secure context that sites cannot load, and whose pages can fetch
    // their own files.
    scheme.setFlags(QWebEngineUrlScheme::SecureScheme | QWebEngineUrlScheme::LocalScheme
                    | QWebEngineUrlScheme::FetchApiAllowed);
    QWebEngineUrlScheme::registerScheme(scheme);
    registered = true;
}

bool isRegistered() { return registered; }

bool isModulePage(const QUrl &url) { return url.scheme() == QLatin1String(schemeName()); }

std::optional<QString> resourceFor(const QUrl &url)
{
    if (url.scheme() != QLatin1String(schemeName()) || !isModuleId(url.host()))
        return std::nullopt;
    const QString path = url.path(QUrl::FullyDecoded);
    if (!path.startsWith(QLatin1Char('/')) || path.contains(QLatin1Char('\\')) || path.contains(QLatin1Char(':')))
        return std::nullopt;
    // Every part a plain name: no "..", no ".", and no empty part, which also
    // refuses a folder.
    const QStringList parts = path.mid(1).split(QLatin1Char('/'));
    for (const QString &part : parts) {
        if (part.isEmpty() || part == QLatin1String(".") || part == QLatin1String(".."))
            return std::nullopt;
    }
    return QStringLiteral(":/modules/%1/web").arg(url.host()) + path;
}

QByteArray contentTypeFor(const QString &resource)
{
    const QString suffix = QFileInfo(resource).suffix().toLower();
    if (suffix == QLatin1String("html"))
        return QByteArrayLiteral("text/html");
    if (suffix == QLatin1String("js"))
        return QByteArrayLiteral("text/javascript");
    if (suffix == QLatin1String("css"))
        return QByteArrayLiteral("text/css");
    if (suffix == QLatin1String("json"))
        return QByteArrayLiteral("application/json");
    if (suffix == QLatin1String("svg"))
        return QByteArrayLiteral("image/svg+xml");
    if (suffix == QLatin1String("png"))
        return QByteArrayLiteral("image/png");
    if (suffix == QLatin1String("jpg") || suffix == QLatin1String("jpeg"))
        return QByteArrayLiteral("image/jpeg");
    if (suffix == QLatin1String("webp"))
        return QByteArrayLiteral("image/webp");
    if (suffix == QLatin1String("woff2"))
        return QByteArrayLiteral("font/woff2");
    return {};
}

bool allowed(const QUrl &initiator, const QUrl &url)
{
    if (initiator.isEmpty())
        return true;
    return initiator.scheme() == QLatin1String(schemeName()) && initiator.host() == url.host();
}

QMultiMap<QByteArray, QByteArray> responseHeaders()
{
    QMultiMap<QByteArray, QByteArray> headers;
    // default-src does not cover these directives, so each is set too. A
    // module's page talks to its own origin only, and is not framed by others.
    headers.insert(QByteArrayLiteral("Content-Security-Policy"),
                   QByteArrayLiteral("default-src 'self'; connect-src 'self'; object-src 'none'; base-uri 'none'; "
                                     "form-action 'none'; frame-ancestors 'self'"));
    headers.insert(QByteArrayLiteral("X-Content-Type-Options"), QByteArrayLiteral("nosniff"));
    headers.insert(QByteArrayLiteral("Referrer-Policy"), QByteArrayLiteral("no-referrer"));
    // A page changed in a new build loads fresh.
    headers.insert(QByteArrayLiteral("Cache-Control"), QByteArrayLiteral("no-store"));
    return headers;
}

} // namespace modulepages

ModuleSchemeHandler::ModuleSchemeHandler(QObject *parent)
    : QWebEngineUrlSchemeHandler(parent)
{
}

void ModuleSchemeHandler::requestStarted(QWebEngineUrlRequestJob *job)
{
    if (job->requestMethod() != QByteArrayLiteral("GET")
        || !modulepages::allowed(job->initiator(), job->requestUrl())) {
        job->fail(QWebEngineUrlRequestJob::RequestDenied);
        return;
    }
    const std::optional<QString> resource = modulepages::resourceFor(job->requestUrl());
    const QByteArray contentType = resource ? modulepages::contentTypeFor(*resource) : QByteArray();
    if (!resource || contentType.isEmpty()) {
        job->fail(QWebEngineUrlRequestJob::UrlNotFound);
        return;
    }
    // The job owns the file, which must stay open while the job reads it.
    auto *file = new QFile(*resource, job);
    if (!file->open(QIODevice::ReadOnly)) {
        job->fail(QWebEngineUrlRequestJob::UrlNotFound);
        return;
    }
    job->setAdditionalResponseHeaders(modulepages::responseHeaders());
    job->reply(contentType, file);
}

} // namespace mirante
