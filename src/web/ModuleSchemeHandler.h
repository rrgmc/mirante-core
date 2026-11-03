#pragma once

#include <QByteArray>
#include <QMultiMap>
#include <QString>
#include <QWebEngineUrlSchemeHandler>

#include <optional>

class QUrl;

namespace mirante {

// The module:// scheme of the modules' own web pages: module://<module id>/<path>
// serves the resource :/modules/<module id>/web/<path>, so each module's pages
// have an origin of their own. See docs/web-views.md#module-pages and
// docs/decisions/0128-module-pages-have-their-own-origin.md.
namespace modulepages {

// "module".
QByteArray schemeName();

// Registers the scheme with Qt WebEngine. Call it before QApplication is made.
void registerScheme();

// True once registerScheme() was called in this process.
bool isRegistered();

// Whether url is a module:// address, which the external browser cannot open.
bool isModulePage(const QUrl &url);

// The resource that serves an address, such as ":/modules/acme/web/index.html"
// for module://acme/index.html, or nothing for an address the scheme does not
// serve: a host that is not a module id, a path that climbs out with "..", or
// a folder.
std::optional<QString> resourceFor(const QUrl &url);

// The content type of a resource from its suffix, such as "text/html", or
// empty for a suffix the scheme does not serve.
QByteArray contentTypeFor(const QString &resource);

// Whether a page of initiator may load url. A top-level load has no initiator.
// Otherwise only the module's own pages may load its files, so another
// module's page cannot read them.
bool allowed(const QUrl &initiator, const QUrl &url);

// The headers sent with every reply: a Content-Security-Policy that allows the
// page's own origin only, and the others of dash://.
QMultiMap<QByteArray, QByteArray> responseHeaders();

} // namespace modulepages

// Serves module:// from the resources. Each profile of the list installs it,
// so a module's page can use the item's profile; the app profile does not.
class ModuleSchemeHandler : public QWebEngineUrlSchemeHandler
{
    Q_OBJECT

public:
    explicit ModuleSchemeHandler(QObject *parent = nullptr);

    void requestStarted(QWebEngineUrlRequestJob *job) override;
};

} // namespace mirante
