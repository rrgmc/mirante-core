#pragma once

#include <mirante/widgets/StatusPill.h>

#include <QList>
#include <QPoint>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariant>
#include <QWidget>

#include <memory>
#include <optional>

namespace mirante::widgets {

struct ListColumn
{
    QString title;
    // 0 never hides. In a narrow cell, the highest number hides first. A column
    // that holds links takes 0.
    int hidePriority = 0;
    bool stretch = false;
};

struct ListLink
{
    QString id;                         // comes back in rowLinkActivated()
    QString label;
};

struct ListCell
{
    QString text;
    QVariant sortKey;                   // the text if null
    std::optional<StatusRole> role;     // shows the text as a pill
    QList<ListLink> links = {};         // shown instead of the text and the pill
};

struct ListRow
{
    QString key;                        // stable, so the selection survives new rows
    QList<ListCell> cells;
};

// A sortable table, with optional filters above it, pages below it, and links on
// its rows. Sorting and filtering cover all rows, and the pages cut up the result.
// See decision 0071.
class ItemListView : public QWidget
{
    Q_OBJECT

public:
    explicit ItemListView(QWidget *parent = nullptr);
    ~ItemListView() override;

    // Sorting starts on column sortColumn, descending.
    void setColumns(const QList<ListColumn> &columns, int sortColumn);
    void setColumnVisible(int column, bool visible);
    void setRows(const QList<ListRow> &rows);
    void setCell(const QString &key, int column, const ListCell &cell);

    // How many rows show at a time, with Previous and Next under the table. Zero
    // or less means no pages, which is the default. The controls hide when all
    // rows fit. A new sort or filter goes back to the first page. New rows stay
    // on the same page, or on the last one if there are now fewer pages.
    void setPageSize(int rows);
    int pageSize() const;

    // The keys of the current page's rows, in table order.
    QStringList visibleKeys() const;

    // A drop-down above the table for a column, filled from all rows' text in
    // that column, with "All" first. Its values have check boxes, so several
    // can be chosen; a row is shown when its cell has any of them. Choosing an
    // item with setCurrentIndex() chooses that value alone, and index 0 is
    // "All". A chosen value survives setRows() while it exists; with none
    // left, the filter falls back to "All". Its object name is
    // filter.<column>, and it is a QComboBox.
    void addFilter(int column, const QString &label);
    // The values that the column's filter chooses at the start, such as
    // "failure" for a Status filter. Each is chosen once a row has it, until
    // the user changes the filter; then the defaults no longer apply. Call it
    // after addFilter().
    void setFilterDefault(int column, const QStringList &values);

    // A line above the table that says what the list shows, such as "Assigned
    // to me · acme/backend". Empty hides it.
    void setDescription(const QString &text);

    // The "Web" link in a thin row above the table. An empty url hides it.
    // For the item's own page, prefer ItemContext::setWebLink(), which the
    // cell header shows; see decision 0146.
    void setWebLink(const QUrl &url);
    QUrl webLink() const;

    // Shown instead of the table, such as "Loading…". Empty removes it.
    void setMessage(const QString &text);

signals:
    void rowMenuRequested(const QString &key, const QPoint &globalPos);
    // A link in a cell was clicked, or Return was pressed on its row, which
    // activates the row's first link.
    void rowLinkActivated(const QString &key, const QString &linkId);
    // visibleKeys() has a new value, for a view that asks more about those rows.
    void visibleKeysChanged();
    // The user changed the values chosen in a column's filter, such as to
    // keep them for the next start with setFilterDefault(). The values are
    // sorted, and empty means "All". It is not sent for setFilterDefault(),
    // nor when setRows() removes a chosen value.
    void filterChanged(int column, const QStringList &values);

private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace mirante::widgets
