/******************************************************************************
************************* IMPORTANT NOTE -- READ ME!!! ************************
*******************************************************************************
* THIS SOFTWARE IMPLEMENTS A **DRAFT** STANDARD, BSR E1.33 REV. 63. UNDER NO
* CIRCUMSTANCES SHOULD THIS SOFTWARE BE USED FOR ANY PRODUCT AVAILABLE FOR
* GENERAL SALE TO THE PUBLIC. DUE TO THE INEVITABLE CHANGE OF DRAFT PROTOCOL
* VALUES AND BEHAVIORAL REQUIREMENTS, PRODUCTS USING THIS SOFTWARE WILL **NOT**
* BE INTEROPERABLE WITH PRODUCTS IMPLEMENTING THE FINAL RATIFIED STANDARD.
*******************************************************************************
* Copyright 2018 ETC Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************
* This file is a part of RDMnet. For more information, go to:
* https://github.com/ETCLabs/RDMnet
******************************************************************************/

#include "NetworkDetailsProxyModel.h"
#include "SearchingStatusItem.h"
#include "PropertyItem.h"

NetworkDetailsProxyModel::NetworkDetailsProxyModel()
{
  currentParentIndex = NULL;
  currentParentItem = NULL;
  filterEnabled = true;
  setDynamicSortFilter(true);
}

NetworkDetailsProxyModel::~NetworkDetailsProxyModel()
{
}

// QModelIndex NetworkDetailsProxyModel::mapToSource(const QModelIndex &proxyIndex) const
//{
//
//}
//
// QModelIndex NetworkDetailsProxyModel::mapFromSource(const QModelIndex &sourceIndex) const
//{
//
//}
//
// QModelIndex NetworkDetailsProxyModel::index(int row, int column, const QModelIndex &parent) const
//{
//
//}
//
// QModelIndex NetworkDetailsProxyModel::parent(const QModelIndex &child) const
//{
//
//}
//
// int NetworkDetailsProxyModel::rowCount(const QModelIndex &parent) const
//{
//
//}
//
// int NetworkDetailsProxyModel::columnCount(const QModelIndex &parent) const
//{
//
//}

void NetworkDetailsProxyModel::setCurrentParentIndex(const QModelIndex &index)
{
  if (currentParentIndex != NULL)
  {
    delete currentParentIndex;
  }

  currentParentIndex = new QPersistentModelIndex(index);
}

void NetworkDetailsProxyModel::clearCurrentParentIndex()
{
  delete currentParentIndex;
  currentParentIndex = NULL;
}

void NetworkDetailsProxyModel::setCurrentParentItem(const QStandardItem *item)
{
  currentParentItem = item;
  invalidate();  // invalidateFilter();
}

bool NetworkDetailsProxyModel::currentParentIsChildOfOrEqualTo(const QStandardItem *item)
{
  const QStandardItem *currentItem = currentParentItem;

  if ((item != NULL) && (currentParentItem != NULL))
  {
    while (currentItem != NULL)
    {
      if (currentItem == item)
      {
        return true;
      }

      currentItem = currentItem->parent();
    }
  }

  return false;
}

void NetworkDetailsProxyModel::setSourceModel(QAbstractItemModel *sourceModel)
{
  QSortFilterProxyModel::setSourceModel(sourceModel);

  sourceNetworkModel = dynamic_cast<RDMnetNetworkModel *>(sourceModel);
}

void NetworkDetailsProxyModel::setFilterEnabled(bool setting)
{
  filterEnabled = setting;
}

bool NetworkDetailsProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
  if (filterEnabled)
  {
    bool isChildOfCurrentParent = false;

    if (sourceNetworkModel)
    {
      QStandardItem *item = NULL, *potentialParent = NULL;

      if (source_parent.isValid())
      {
        item = sourceNetworkModel->itemFromIndex(source_parent);
      }
      else if (source_parent == sourceNetworkModel->indexFromItem(sourceNetworkModel->invisibleRootItem()))
      {
        item = sourceNetworkModel->invisibleRootItem();
        isChildOfCurrentParent = true;
      }

      potentialParent = item;

      while ((potentialParent != NULL) && !isChildOfCurrentParent)
      {
        isChildOfCurrentParent = (potentialParent == currentParentItem);

        potentialParent = potentialParent->parent();
      }

      if (item && isChildOfCurrentParent)
      {
        QStandardItem *child = item->child(source_row);

        if (child)
        {
          if (child->type() == PropertyItem::PropertyItemType)
          {
            return true;
          }
        }
      }
    }

    return !isChildOfCurrentParent;
  }

  return true;
}

bool NetworkDetailsProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
  QVariant leftData = sourceModel()->data(left);
  QVariant rightData = sourceModel()->data(right);

  return (leftData.toString() < rightData.toString());
}