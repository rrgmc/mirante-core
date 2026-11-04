#pragma once

#include <mirante/UrlMatch.h>

#include <QString>
#include <QUrl>

#include <vector>

namespace mirante {

class ItemType;
class ItemTypeRegistry;
class Model;

// Asks the item types what they make of a pasted web address. The new item
// dialog and quick add use it. See decisions 0049 and 0141. Implemented in
// UrlMatches.cpp.
namespace urlmatches {

// The text as a web address: http or https, with a host. Spaces around it
// are ignored. An empty URL for any other text.
QUrl webAddress(const QString &text);

// One type that takes the address, with what it made of it.
struct TypeMatch
{
    const ItemType *type = nullptr;
    UrlMatch match;
    QString accountId;   // the account it was read with; empty for the default
};

// The account of the module's server in the profile that a link to url
// belongs to: the first whose base URL is on the same host, the default
// account first, then the others by name. Else the default, whose id is
// empty. See decision 0144.
QString accountFor(const ItemTypeRegistry &types, const Model &model, const QString &profileId,
                   const QString &moduleId, const QUrl &url);

// The types of the registry that take url, best first: the types of modules
// before generic ones, such as Web page, and otherwise by name. profileId is
// the profile the new item would use; each type reads its servers there, in
// the account accountFor() picks. Without a model, in the default account.
std::vector<TypeMatch> match(const ItemTypeRegistry &types, const Model *model, const QUrl &url,
                             const QString &profileId);

} // namespace urlmatches
} // namespace mirante
