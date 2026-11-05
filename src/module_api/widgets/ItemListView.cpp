#include <mirante/widgets/ItemListView.h>

#include <mirante/widgets/ActionRow.h>
#include <mirante/widgets/FlowRow.h>

#include <QAbstractTableModel>
#include <QApplication>
#include <QComboBox>
#include <QEvent>
#include <QFontMetricsF>
#include <QHash>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPersistentModelIndex>
#include <QPushButton>
#include <QScrollBar>
#include <QSet>
#include <QSortFilterProxyModel>
#include <QStyleOptionComboBox>
#include <QStyledItemDelegate>
#include <QStylePainter>
#include <QTableView>
#include <QTextDocument>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <functional>
#include <utility>

namespace mirante::widgets {

namespace {

enum Role { SortKeyRole = Qt::UserRole, RowKeyRole, StatusColoursRole, LinkLabelsRole, LinkIdsRole };

// The least room a stretching column, such as the title, needs before other
// columns start to hide.
constexpr int stretchColumnWidth = 160;
// The margins of a header section and its sort arrow.
constexpr int headerPadding = 28;
constexpr int pillPadding = 8;
constexpr int pillMargin = 4;
constexpr int pillVerticalMargin = 3;
constexpr int linkMargin = 6;
constexpr int linkSpacing = 12;

// Where a cell draws its links, one rectangle per label. The drawing and the
// mouse both use it.
QList<QRect> linkRects(const QRect &cell, const QFontMetrics &metrics, const QStringList &labels)
{
    QList<QRect> rects;
    int left = cell.left() + linkMargin;
    for (const QString &label : labels) {
        const int width = metrics.horizontalAdvance(label);
        rects.append(QRect(left, cell.top(), width, cell.height()));
        left += width + linkSpacing;
    }
    return rects;
}

class ListModel : public QAbstractTableModel
{
public:
    using QAbstractTableModel::QAbstractTableModel;

    void setColumns(const QList<ListColumn> &columns)
    {
        beginResetModel();
        m_columns = columns;
        endResetModel();
    }

    const QList<ListColumn> &columns() const { return m_columns; }

    // Changes the rows in place and sends no reset, which would make the view
    // and its header start over: scroll position, column sizes and hidden
    // columns. The caller puts the selection right afterwards.
    void setRows(const QList<ListRow> &rows)
    {
        const int oldCount = int(m_rows.size());
        const int newCount = int(rows.size());
        if (newCount < oldCount) {
            beginRemoveRows({}, newCount, oldCount - 1);
            m_rows.resize(newCount);
            endRemoveRows();
        }
        const int shared = std::min(oldCount, newCount);
        for (int row = 0; row < shared; ++row)
            m_rows[row] = rows[row];
        if (shared > 0 && !m_columns.isEmpty())
            emit dataChanged(index(0, 0), index(shared - 1, columnCount() - 1));
        if (newCount > oldCount) {
            beginInsertRows({}, oldCount, newCount - 1);
            m_rows.append(rows.mid(oldCount));
            endInsertRows();
        }
    }

    void setCell(const QString &key, int column, const ListCell &cell)
    {
        const int row = rowOf(key);
        if (row < 0 || column < 0 || column >= columnCount())
            return;
        QList<ListCell> &cells = m_rows[row].cells;
        if (cells.size() <= column)
            cells.resize(column + 1);
        cells[column] = cell;
        emit dataChanged(index(row, column), index(row, column));
    }

    int rowOf(const QString &key) const
    {
        for (int row = 0; row < m_rows.size(); ++row) {
            if (m_rows[row].key == key)
                return row;
        }
        return -1;
    }

    // The different texts of a column, in reading order, without empty ones.
    QStringList valuesOf(int column) const
    {
        QSet<QString> values;
        for (const ListRow &row : m_rows) {
            const QString text = row.cells.value(column).text;
            if (!text.isEmpty())
                values.insert(text);
        }
        QStringList sorted(values.cbegin(), values.cend());
        std::sort(sorted.begin(), sorted.end(), [](const QString &a, const QString &b) {
            const int order = a.compare(b, Qt::CaseInsensitive);
            return order != 0 ? order < 0 : a < b;
        });
        return sorted;
    }

    int rowCount(const QModelIndex &parent = {}) const override
    {
        return parent.isValid() ? 0 : int(m_rows.size());
    }

    int columnCount(const QModelIndex &parent = {}) const override
    {
        return parent.isValid() ? 0 : int(m_columns.size());
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid())
            return {};
        const ListRow &row = m_rows[index.row()];
        if (role == RowKeyRole)
            return row.key;
        const ListCell cell = row.cells.value(index.column());
        switch (role) {
        case Qt::DisplayRole:
            return cell.text;
        case Qt::ToolTipRole:
            // A tooltip reads text that looks like HTML as markup, and this
            // text comes from a server.
            return cell.text.isEmpty() ? QString() : Qt::convertFromPlainText(cell.text);
        case SortKeyRole:
            return cell.sortKey.isNull() ? QVariant(cell.text) : cell.sortKey;
        case StatusColoursRole:
            return cell.role ? QVariant(int(*cell.role)) : QVariant();
        case LinkLabelsRole:
        case LinkIdsRole: {
            QStringList values;
            for (const ListLink &link : cell.links)
                values.append(role == LinkLabelsRole ? link.label : link.id);
            return values;
        }
        default:
            return {};
        }
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole && section >= 0
            && section < m_columns.size())
            return m_columns[section].title;
        return {};
    }

private:
    QList<ListColumn> m_columns;
    QList<ListRow> m_rows;
};

// Sorts the rows and keeps those that match the filters' choices. A choice
// compares with a cell's text, as the drop-down shows it. A row matches a
// filter when its cell has any of the chosen values.
class ListProxy : public QSortFilterProxyModel
{
public:
    using QSortFilterProxyModel::QSortFilterProxyModel;

    // No values take the filter off.
    void setChoice(int column, const QSet<QString> &values)
    {
        if (values.isEmpty())
            m_choices.remove(column);
        else
            m_choices.insert(column, values);
        invalidateFilter();
    }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
    {
        for (auto choice = m_choices.cbegin(); choice != m_choices.cend(); ++choice) {
            const QModelIndex cell = sourceModel()->index(sourceRow, choice.key(), sourceParent);
            if (!choice.value().contains(cell.data().toString()))
                return false;
        }
        return true;
    }

private:
    QHash<int, QSet<QString>> m_choices;
};

// A filter's drop-down: "All" first, then each value with a check box.
// Clicking a value in the open list checks or unchecks it and leaves the list
// open, so several values can be chosen. Clicking "All" unchecks them all.
// Choosing an item any other way, such as with the keyboard, chooses its value
// alone. With nothing checked the filter is off, and "All" is checked.
class FilterChoice : public QComboBox
{
public:
    explicit FilterChoice(QWidget *parent)
        : QComboBox(parent)
    {
        // The styled delegate draws check boxes in every style, where the
        // combo box's own one may not.
        setItemDelegate(new QStyledItemDelegate(this));
        view()->viewport()->installEventFilter(this);
        connect(this, &QComboBox::currentIndexChanged, this, [this](int index) {
            choose(index > 0 ? QSet<QString>{itemData(index).toString()} : QSet<QString>());
        });
    }

    // Called after the user changed the chosen values.
    std::function<void()> changed;

    QSet<QString> chosen() const { return m_chosen; }

    QStringList values() const
    {
        QStringList values;
        for (int item = 1; item < count(); ++item)
            values.append(itemData(item).toString());
        return values;
    }

    // Fills the list. Chosen values that are gone are no longer chosen, and do
    // not come back with their value. Until the user chooses, the chosen values
    // are the defaults that the list has. It does not call changed.
    void setValues(const QStringList &values)
    {
        const QSignalBlocker blocker(this);
        clear();
        addItem(ItemListView::tr("All"));
        for (const QString &value : values)
            addItem(value, value);
        if (!m_userChose)
            m_chosen = m_defaults;
        m_chosen.intersect(QSet<QString>(values.cbegin(), values.cend()));
        showChosen();
    }

    // The values chosen while the user has not chosen any. It does not call
    // changed.
    void setDefaults(const QSet<QString> &values)
    {
        m_defaults = values;
        setValues(this->values());
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        // Runs before the popup's own filter, which would close it.
        if (watched == view()->viewport() && event->type() == QEvent::MouseButtonRelease) {
            const QModelIndex index = view()->indexAt(static_cast<QMouseEvent *>(event)->position().toPoint());
            if (index.row() > 0) {
                QSet<QString> next = m_chosen;
                const QString value = itemData(index.row()).toString();
                if (!next.remove(value))
                    next.insert(value);
                choose(next);
                return true;
            }
        }
        return QComboBox::eventFilter(watched, event);
    }

    // Shows several chosen values joined, as many as fit.
    void paintEvent(QPaintEvent *) override
    {
        QStylePainter painter(this);
        QStyleOptionComboBox option;
        initStyleOption(&option);
        if (m_chosen.size() > 1) {
            const QRect field = style()->subControlRect(QStyle::CC_ComboBox, &option,
                                                        QStyle::SC_ComboBoxEditField, this);
            option.currentText = fontMetrics().elidedText(chosenText(), Qt::ElideRight, field.width());
            option.currentIcon = QIcon();
        }
        painter.drawComplexControl(QStyle::CC_ComboBox, option);
        painter.drawControl(QStyle::CE_ComboBoxLabel, option);
    }

private:
    void choose(const QSet<QString> &values)
    {
        m_userChose = true;
        if (values == m_chosen)
            return;
        m_chosen = values;
        showChosen();
        if (changed)
            changed();
    }

    // The chosen values in list order.
    QString chosenText() const
    {
        QStringList names;
        for (const QString &value : values()) {
            if (m_chosen.contains(value))
                names.append(value);
        }
        return names.join(QStringLiteral(", "));
    }

    void showChosen()
    {
        const QSignalBlocker blocker(this);
        for (int item = 0; item < count(); ++item) {
            const bool checked = item == 0 ? m_chosen.isEmpty() : m_chosen.contains(itemData(item).toString());
            setItemData(item, checked ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);
        }
        // Several values have no current item; paintEvent shows them.
        if (m_chosen.isEmpty())
            setCurrentIndex(0);
        else if (m_chosen.size() == 1)
            setCurrentIndex(findData(*m_chosen.cbegin()));
        else
            setCurrentIndex(-1);
        setToolTip(m_chosen.size() > 1 ? chosenText() : QString());
        update();
    }

    QSet<QString> m_chosen;
    QSet<QString> m_defaults;
    // Once the user chooses, the defaults no longer apply.
    bool m_userChose = false;
};

// Draws a cell with links as links, and a cell with a status role as a pill.
// Other cells are drawn as usual.
class CellDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    // The link under the mouse. A negative link means none.
    void setHovered(const QModelIndex &index, int link)
    {
        m_hoveredIndex = index;
        m_hoveredLink = link;
    }

    QModelIndex hoveredIndex() const { return m_hoveredIndex; }
    int hoveredLink() const { return m_hoveredLink; }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        const QStringList labels = index.data(LinkLabelsRole).toStringList();
        if (!labels.isEmpty()) {
            paintLinks(painter, option, index, labels);
            return;
        }

        const QVariant role = index.data(StatusColoursRole);
        if (!role.isValid()) {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }

        QStyleOptionViewItem cell(option);
        initStyleOption(&cell, index);
        const QString text = cell.text;
        // The background and the selection, without the text.
        cell.text.clear();
        QStyle *style = cell.widget ? cell.widget->style() : QApplication::style();
        style->drawControl(QStyle::CE_ItemViewItem, &cell, painter, cell.widget);

        const StatusColours colours =
                statusColours(StatusRole(role.toInt()), option.palette);
        const QRect available = option.rect.adjusted(pillMargin, pillVerticalMargin, -pillMargin,
                                                     -pillVerticalMargin);
        const int wanted = option.fontMetrics.horizontalAdvance(text) + 2 * pillPadding;
        const QRect pill(available.topLeft(),
                         QSize(std::min(wanted, available.width()), available.height()));

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(Qt::NoPen);
        painter->setBrush(colours.fill);
        painter->drawRoundedRect(pill, pill.height() / 2.0, pill.height() / 2.0);
        const QRect textArea = pill.adjusted(pillPadding, 0, -pillPadding, 0);
        painter->setPen(colours.text);
        painter->drawText(textArea, Qt::AlignCenter,
                          option.fontMetrics.elidedText(text, Qt::ElideRight, textArea.width()));
        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        const QStringList labels = index.data(LinkLabelsRole).toStringList();
        if (!labels.isEmpty()) {
            const QRect last = linkRects(QRect(), option.fontMetrics, labels).last();
            size.setWidth(last.left() + last.width() + linkMargin);
        } else if (index.data(StatusColoursRole).isValid()) {
            // Rounded up, because horizontalAdvance() rounds a fractional width
            // down on macOS, and the text then elides by a pixel.
            const int wanted = static_cast<int>(std::ceil(
                    QFontMetricsF(option.font).horizontalAdvance(index.data().toString())));
            size.setWidth(wanted + 2 * (pillPadding + pillMargin) + 1);
        }
        return size;
    }

private:
    void paintLinks(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index,
                    const QStringList &labels) const
    {
        QStyleOptionViewItem cell(option);
        initStyleOption(&cell, index);
        // The background and the selection, without the text.
        cell.text.clear();
        QStyle *style = cell.widget ? cell.widget->style() : QApplication::style();
        style->drawControl(QStyle::CE_ItemViewItem, &cell, painter, cell.widget);

        painter->save();
        painter->setClipRect(option.rect);
        painter->setPen(option.palette.color((option.state & QStyle::State_Selected)
                                                     ? QPalette::HighlightedText
                                                     : QPalette::Link));
        const QList<QRect> rects = linkRects(option.rect, option.fontMetrics, labels);
        for (int link = 0; link < labels.size(); ++link) {
            QFont font = option.font;
            font.setUnderline(index == m_hoveredIndex && link == m_hoveredLink);
            painter->setFont(font);
            painter->drawText(rects[link], Qt::AlignVCenter | Qt::AlignLeft | Qt::TextSingleLine,
                              labels[link]);
        }
        painter->restore();
    }

    QPersistentModelIndex m_hoveredIndex;
    int m_hoveredLink = -1;
};

} // namespace

struct ItemListView::Private : public QObject
{
    struct Filter
    {
        int column = 0;
        FilterChoice *choice = nullptr;
    };

    ItemListView *q = nullptr;
    ListModel *model = nullptr;
    ListProxy *proxy = nullptr;
    QWidget *tableArea = nullptr;
    QTableView *table = nullptr;
    CellDelegate *delegate = nullptr;
    QWidget *filterRow = nullptr;
    FlowRow *filterFlow = nullptr;
    QList<Filter> filters;
    QWidget *pageRow = nullptr;
    QLabel *pageLine = nullptr;
    QPushButton *previousPage = nullptr;
    QPushButton *nextPage = nullptr;
    int pageSize = 0;
    int page = 0;
    QStringList visibleKeys;
    // The link under the mouse button, so that a click starts and ends on it.
    QPersistentModelIndex pressedIndex;
    int pressedLink = -1;
    QLabel *description = nullptr;
    ActionRow *webRow = nullptr;
    QLabel *message = nullptr;
    QSet<int> columnsHiddenByView;
    // While rows change, the page waits, since the number of rows can pass
    // through a low point.
    bool changingRows = false;

    QString selectedKey() const
    {
        const QModelIndexList selected = table->selectionModel()->selectedIndexes();
        return selected.isEmpty() ? QString() : selected.first().data(RowKeyRole).toString();
    }

    void select(const QString &key)
    {
        const int row = key.isEmpty() ? -1 : model->rowOf(key);
        // A row that a filter removes has no index in the table.
        const QModelIndex index = row < 0 ? QModelIndex() : proxy->mapFromSource(model->index(row, 0));
        if (!index.isValid()) {
            table->selectionModel()->clear();
            return;
        }
        table->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect
                                                                | QItemSelectionModel::Rows);
    }

    // Makes a change to the rows or to which of them show. The selection stays on
    // its row, so that Return acts on the row the user chose. The view does not
    // move, although selecting a row scrolls to it.
    template<typename Change>
    void changeRows(Change change)
    {
        const QString key = selectedKey();
        const int position = table->verticalScrollBar()->value();

        changingRows = true;
        change();
        select(key);
        changingRows = false;

        updatePage();
        table->verticalScrollBar()->setValue(position);
        updateColumns();
    }

    // Pages hide the table's other rows, and do not take them from the model. So
    // the selection stays on its row across pages, and sorting and filtering see
    // all rows without any extra work.
    void updatePage()
    {
        if (changingRows)
            return;
        const int total = proxy->rowCount();
        const int size = pageSize > 0 ? pageSize : std::max(total, 1);
        const int lastPage = total > 0 ? (total - 1) / size : 0;
        page = std::clamp(page, 0, lastPage);
        const int first = page * size;
        const int end = std::min(first + size, total);

        bool changed = false;
        for (int row = 0; row < total; ++row) {
            const bool hidden = row < first || row >= end;
            if (table->isRowHidden(row) != hidden) {
                table->setRowHidden(row, hidden);
                changed = true;
            }
        }
        if (changed) {
            // Columns that fit their contents measure only the rows that show,
            // and hiding rows does not make them measure again.
            table->horizontalHeader()->doItemsLayout();
            updateColumns();
        }

        pageRow->setVisible(total > size);
        pageLine->setText(ItemListView::tr("%1-%2 of %3").arg(first + 1).arg(end).arg(total));
        previousPage->setEnabled(page > 0);
        nextPage->setEnabled(page < lastPage);

        QStringList keys;
        for (int row = first; row < end; ++row)
            keys.append(proxy->index(row, 0).data(RowKeyRole).toString());
        if (keys != visibleKeys) {
            visibleKeys = keys;
            emit q->visibleKeysChanged();
        }
    }

    void goToPage(int wanted)
    {
        page = wanted;
        updatePage();
        table->scrollToTop();
    }

    // Fills the drop-downs from the rows. A list that has not changed stays as it
    // is, so that new rows do not disturb an open drop-down.
    void updateFilters()
    {
        for (const Filter &filter : std::as_const(filters)) {
            const QStringList values = model->valuesOf(filter.column);
            if (values == filter.choice->values() && filter.choice->count() > 0)
                continue;

            const QSet<QString> chosen = filter.choice->chosen();
            filter.choice->setValues(values);
            if (filter.choice->chosen() != chosen) {
                proxy->setChoice(filter.column, filter.choice->chosen());
                page = 0;
            }
        }
    }

    // Which link of a cell is at a point in the viewport, or -1.
    int linkAt(const QModelIndex &index, const QPoint &pos) const
    {
        const QStringList labels = index.data(LinkLabelsRole).toStringList();
        const QList<QRect> rects = linkRects(table->visualRect(index), table->fontMetrics(), labels);
        for (int link = 0; link < rects.size(); ++link) {
            if (rects[link].contains(pos))
                return link;
        }
        return -1;
    }

    void updateHover(const QModelIndex &index, int link)
    {
        const QModelIndex old = delegate->hoveredIndex();
        if (index == old && link == delegate->hoveredLink())
            return;
        delegate->setHovered(index, link);
        if (old.isValid())
            table->viewport()->update(table->visualRect(old));
        if (index.isValid())
            table->viewport()->update(table->visualRect(index));
        if (link >= 0)
            table->viewport()->setCursor(Qt::PointingHandCursor);
        else
            table->viewport()->unsetCursor();
    }

    void activateLink(const QModelIndex &index, int link)
    {
        const QString id = index.data(LinkIdsRole).toStringList().value(link);
        emit q->rowLinkActivated(index.data(RowKeyRole).toString(), id);
    }

    // Return on a row activates its first link. The others are in the row menu.
    bool activateFirstLinkOfCurrentRow()
    {
        const QModelIndex current = table->currentIndex();
        if (!current.isValid() || table->isRowHidden(current.row()))
            return false;
        for (int column = 0; column < proxy->columnCount(); ++column) {
            const QModelIndex cell = current.siblingAtColumn(column);
            if (!cell.data(LinkIdsRole).toStringList().isEmpty()) {
                activateLink(cell, 0);
                return true;
            }
        }
        return false;
    }

    void mouseOnViewport(QEvent *event)
    {
        const auto *mouse = static_cast<QMouseEvent *>(event);
        const QPoint pos = mouse->position().toPoint();
        const QModelIndex index = table->indexAt(pos);
        const int link = index.isValid() ? linkAt(index, pos) : -1;
        const QModelIndex hit = link >= 0 ? index : QModelIndex();
        updateHover(hit, link);

        if (mouse->button() != Qt::LeftButton)
            return;
        if (event->type() == QEvent::MouseButtonPress) {
            pressedIndex = hit;
            pressedLink = link;
        } else if (event->type() == QEvent::MouseButtonRelease) {
            const bool sameLink = link >= 0 && hit == QModelIndex(pressedIndex) && link == pressedLink;
            pressedIndex = QModelIndex();
            pressedLink = -1;
            if (sameLink)
                activateLink(hit, link);
        }
    }

    int neededWidth(int column) const
    {
        // QTableView makes this function protected, but the base class has it public.
        const QAbstractItemView *view = table;
        // Measured from the title, because sectionSizeHint() gives 0 for a hidden
        // section, and the result must not depend on what is hidden now.
        const QHeaderView *header = table->horizontalHeader();
        const int headerWidth = header->fontMetrics().horizontalAdvance(model->columns()[column].title)
                + headerPadding;
        const int contentWidth = view->sizeHintForColumn(column);
        // A stretching column counts only up to stretchColumnWidth. Longer text
        // is elided, so that one long title does not hide the other columns.
        if (model->columns()[column].stretch)
            return std::max(headerWidth, std::min(contentWidth, stretchColumnWidth));
        return std::max(headerWidth, contentWidth);
    }

    // Hides columns by priority until the rest fit. It always starts from all
    // columns, so that the result depends only on the width.
    void updateColumns()
    {
        const QList<ListColumn> &columns = model->columns();
        QList<int> hideable;
        int needed = 0;
        for (int column = 0; column < columns.size(); ++column) {
            if (columnsHiddenByView.contains(column))
                continue;
            needed += neededWidth(column);
            if (columns[column].hidePriority > 0)
                hideable.append(column);
        }
        std::stable_sort(hideable.begin(), hideable.end(), [&columns](int a, int b) {
            return columns[a].hidePriority > columns[b].hidePriority;
        });

        QSet<int> tooNarrow;
        const int available = table->viewport()->width();
        for (int column : std::as_const(hideable)) {
            if (needed <= available)
                break;
            needed -= neededWidth(column);
            tooNarrow.insert(column);
        }
        for (int column = 0; column < columns.size(); ++column) {
            table->setColumnHidden(column, columnsHiddenByView.contains(column)
                                                   || tooNarrow.contains(column));
        }
    }

    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (watched == table->viewport()) {
            switch (event->type()) {
            case QEvent::Resize:
                updateColumns();
                break;
            case QEvent::MouseMove:
            case QEvent::MouseButtonPress:
            case QEvent::MouseButtonRelease:
                mouseOnViewport(event);
                break;
            case QEvent::Leave:
                updateHover({}, -1);
                break;
            default:
                break;
            }
        } else if (watched == table && event->type() == QEvent::KeyPress) {
            const int key = static_cast<QKeyEvent *>(event)->key();
            if ((key == Qt::Key_Return || key == Qt::Key_Enter) && activateFirstLinkOfCurrentRow())
                return true;
        }
        return QObject::eventFilter(watched, event);
    }
};

ItemListView::ItemListView(QWidget *parent)
    : QWidget(parent)
    , d(std::make_unique<Private>())
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    d->q = this;

    d->description = new QLabel(this);
    d->description->setObjectName(QStringLiteral("description"));
    d->description->setTextFormat(Qt::PlainText);
    d->description->setWordWrap(true);
    layout->addWidget(d->description);
    d->description->hide();

    d->webRow = new ActionRow(this);
    layout->addWidget(d->webRow);
    d->webRow->hide();

    d->model = new ListModel(this);
    d->proxy = new ListProxy(this);
    d->proxy->setSourceModel(d->model);
    d->proxy->setSortRole(SortKeyRole);
    d->proxy->setSortCaseSensitivity(Qt::CaseInsensitive);

    d->tableArea = new QWidget(this);
    layout->addWidget(d->tableArea, 1);
    auto *tableLayout = new QVBoxLayout(d->tableArea);
    tableLayout->setContentsMargins(0, 0, 0, 0);

    d->filterRow = new QWidget(d->tableArea);
    d->filterFlow = new FlowRow(d->filterRow);
    tableLayout->addWidget(d->filterRow);
    d->filterRow->hide();

    d->table = new QTableView(d->tableArea);
    tableLayout->addWidget(d->table, 1);
    d->table->setModel(d->proxy);
    d->delegate = new CellDelegate(d->table);
    d->table->setItemDelegate(d->delegate);
    d->table->setSelectionBehavior(QAbstractItemView::SelectRows);
    d->table->setSelectionMode(QAbstractItemView::SingleSelection);
    d->table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    d->table->setShowGrid(false);
    d->table->setWordWrap(false);
    d->table->setAlternatingRowColors(true);
    d->table->setSortingEnabled(true);
    d->table->verticalHeader()->hide();
    d->table->setContextMenuPolicy(Qt::CustomContextMenu);
    d->table->viewport()->setMouseTracking(true);
    d->table->viewport()->installEventFilter(d.get());
    d->table->installEventFilter(d.get());

    d->pageRow = new QWidget(d->tableArea);
    auto *pageLayout = new QHBoxLayout(d->pageRow);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    d->pageLine = new QLabel(d->pageRow);
    d->pageLine->setObjectName(QStringLiteral("pageLine"));
    d->pageLine->setTextFormat(Qt::PlainText);
    pageLayout->addWidget(d->pageLine, 1);
    d->previousPage = new QPushButton(tr("Previous"), d->pageRow);
    d->previousPage->setObjectName(QStringLiteral("previousPage"));
    pageLayout->addWidget(d->previousPage);
    d->nextPage = new QPushButton(tr("Next"), d->pageRow);
    d->nextPage->setObjectName(QStringLiteral("nextPage"));
    pageLayout->addWidget(d->nextPage);
    tableLayout->addWidget(d->pageRow);
    d->pageRow->hide();
    connect(d->previousPage, &QPushButton::clicked, d.get(), [this] { d->goToPage(d->page - 1); });
    connect(d->nextPage, &QPushButton::clicked, d.get(), [this] { d->goToPage(d->page + 1); });

    // A new sort starts from its top.
    connect(d->table->horizontalHeader(), &QHeaderView::sortIndicatorChanged, d.get(),
            [this] { d->goToPage(0); });
    connect(d->proxy, &QAbstractItemModel::layoutChanged, d.get(), [this] { d->updatePage(); });
    connect(d->proxy, &QAbstractItemModel::modelReset, d.get(), [this] { d->updatePage(); });
    connect(d->proxy, &QAbstractItemModel::rowsInserted, d.get(), [this] { d->updatePage(); });
    connect(d->proxy, &QAbstractItemModel::rowsRemoved, d.get(), [this] { d->updatePage(); });

    connect(d->table, &QWidget::customContextMenuRequested, d.get(), [this](const QPoint &pos) {
        const QModelIndex index = d->table->indexAt(pos);
        if (index.isValid())
            emit rowMenuRequested(index.data(RowKeyRole).toString(),
                                  d->table->viewport()->mapToGlobal(pos));
    });

    d->message = new QLabel(this);
    d->message->setTextFormat(Qt::PlainText);
    d->message->setWordWrap(true);
    d->message->setAlignment(Qt::AlignCenter);
    layout->addWidget(d->message, 1);
    d->message->hide();
}

ItemListView::~ItemListView() = default;

void ItemListView::setColumns(const QList<ListColumn> &columns, int sortColumn)
{
    d->columnsHiddenByView.clear();
    d->model->setColumns(columns);

    QHeaderView *header = d->table->horizontalHeader();
    bool anyStretch = false;
    for (int column = 0; column < columns.size(); ++column) {
        anyStretch = anyStretch || columns[column].stretch;
        header->setSectionResizeMode(column, columns[column].stretch ? QHeaderView::Stretch
                                                                     : QHeaderView::ResizeToContents);
    }
    header->setStretchLastSection(!anyStretch);
    d->table->sortByColumn(sortColumn, Qt::DescendingOrder);
    d->changeRows([this] { d->updateFilters(); });
}

void ItemListView::setColumnVisible(int column, bool visible)
{
    if (visible)
        d->columnsHiddenByView.remove(column);
    else
        d->columnsHiddenByView.insert(column);
    d->updateColumns();
}

void ItemListView::setRows(const QList<ListRow> &rows)
{
    d->changeRows([this, &rows] {
        d->model->setRows(rows);
        d->updateFilters();
    });
}

void ItemListView::setCell(const QString &key, int column, const ListCell &cell)
{
    d->changeRows([this, &key, column, &cell] {
        d->model->setCell(key, column, cell);
        d->updateFilters();
    });
}

void ItemListView::setPageSize(int rows)
{
    const int size = std::max(rows, 0);
    if (size == d->pageSize)
        return;
    d->pageSize = size;
    d->goToPage(0);
}

int ItemListView::pageSize() const { return d->pageSize; }

QStringList ItemListView::visibleKeys() const { return d->visibleKeys; }

void ItemListView::addFilter(int column, const QString &label)
{
    const bool known = std::any_of(d->filters.cbegin(), d->filters.cend(),
                                   [column](const Private::Filter &filter) { return filter.column == column; });
    Q_ASSERT_X(!known, "ItemListView", "a column has two filters");
    if (known)
        return;

    auto *holder = new QWidget(d->filterRow);
    auto *holderLayout = new QHBoxLayout(holder);
    holderLayout->setContentsMargins(0, 0, 0, 0);
    auto *caption = new QLabel(label, holder);
    caption->setTextFormat(Qt::PlainText);
    holderLayout->addWidget(caption);
    auto *choice = new FilterChoice(holder);
    choice->setObjectName(QStringLiteral("filter.%1").arg(column));
    caption->setBuddy(choice);
    holderLayout->addWidget(choice);
    d->filterFlow->addWidget(holder);
    holder->show();
    d->filterRow->show();

    d->filters.append({column, choice});
    d->updateFilters();
    choice->changed = [this, column, choice] {
        d->changeRows([this, column, choice] {
            d->proxy->setChoice(column, choice->chosen());
            d->page = 0;
        });
        d->table->scrollToTop();
        const QSet<QString> chosen = choice->chosen();
        QStringList values(chosen.cbegin(), chosen.cend());
        values.sort();
        emit filterChanged(column, values);
    };
}

void ItemListView::setFilterDefault(int column, const QStringList &values)
{
    const auto filter = std::find_if(d->filters.cbegin(), d->filters.cend(),
                                     [column](const Private::Filter &filter) { return filter.column == column; });
    Q_ASSERT_X(filter != d->filters.cend(), "ItemListView", "a default for a column without a filter");
    if (filter == d->filters.cend())
        return;

    FilterChoice *choice = filter->choice;
    d->changeRows([this, column, choice, &values] {
        choice->setDefaults(QSet<QString>(values.cbegin(), values.cend()));
        d->proxy->setChoice(column, choice->chosen());
        d->page = 0;
    });
}

void ItemListView::setDescription(const QString &text)
{
    d->description->setText(text);
    d->description->setVisible(!text.isEmpty());
}

void ItemListView::setWebLink(const QUrl &url)
{
    d->webRow->setWebLink(url);
    d->webRow->setVisible(!url.isEmpty());
}

QUrl ItemListView::webLink() const { return d->webRow->webLink(); }

void ItemListView::setMessage(const QString &text)
{
    d->message->setText(text);
    d->message->setVisible(!text.isEmpty());
    d->tableArea->setVisible(text.isEmpty());
}

} // namespace mirante::widgets
