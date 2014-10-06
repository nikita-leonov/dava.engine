#include "ItemDelegateForString.h"
#include <QLineEdit>
#include <QLayout>
#include "DAVAEngine.h"
#include "PropertiesTreeModel.h"
#include "Utils/QtDavaConvertion.h"
#include "PropertiesTreeItemDelegate.h"

ItemDelegateForString::ItemDelegateForString(PropertiesTreeItemDelegate *delegate)
    : PropertyAbstractEditor(delegate)
{

}

ItemDelegateForString::~ItemDelegateForString()
{

}

QWidget *ItemDelegateForString::createEditor( QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    QLineEdit *lineEdit = new QLineEdit(parent);
    lineEdit->setObjectName(QString::fromUtf8("lineEdit"));
    connect(lineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(OnValueChanged()));

    return lineEdit;
}

void ItemDelegateForString::setEditorData( QWidget *rawEditor, const QModelIndex & index ) const 
{
    QLineEdit *editor = rawEditor->findChild<QLineEdit*>("lineEdit");

    DAVA::VariantType variant = index.data(Qt::EditRole).value<DAVA::VariantType>();
    QString stringValue;
    if (variant.GetType() == DAVA::VariantType::TYPE_STRING)
    {
        stringValue = StringToQString(variant.AsString());
    }
    else
    {
        stringValue = WideStringToQString(variant.AsWideString());
    }
    editor->setText(stringValue);
}

bool ItemDelegateForString::setModelData( QWidget * rawEditor, QAbstractItemModel * model, const QModelIndex & index ) const 
{
    if (PropertyAbstractEditor::setModelData(rawEditor, model, index))
        return true;

    QLineEdit *editor = rawEditor->findChild<QLineEdit*>("lineEdit");

    DAVA::VariantType variantType = index.data(Qt::EditRole).value<DAVA::VariantType>();

    if (variantType.GetType() == DAVA::VariantType::TYPE_STRING)
    {
        variantType.SetString(QStringToString(editor->text()));
    }
    else
    {
        variantType.SetWideString(QStringToWideString(editor->text()));
    }

    QVariant variant;
    variant.setValue<DAVA::VariantType>(variantType);

    return model->setData(index, variant, Qt::EditRole);
}

void ItemDelegateForString::OnValueChanged()
{
    QWidget *lineEdit = qobject_cast<QWidget *>(sender());
    if (!lineEdit)
        return;

    QWidget *editor = lineEdit->parentWidget();
    if (!editor)
        return;

    PropertyAbstractEditor::SetValueModified(editor, true);
    itemDelegate->emitCommitData(editor);
}