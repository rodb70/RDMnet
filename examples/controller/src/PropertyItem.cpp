/******************************************************************************
 * Copyright 2020 ETC Inc.
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
 ******************************************************************************
 * This file is a part of RDMnet. For more information, go to:
 * https://github.com/ETCLabs/RDMnet
 *****************************************************************************/

#include "PropertyItem.h"

PropertyItem::PropertyItem(const QString& fullName, const QString& displayText)
    : RDMnetNetworkItem(displayText), m_ValueItem(nullptr)
{
  m_FullName = fullName;
}

PropertyItem::~PropertyItem()
{
}

int PropertyItem::type() const
{
  return PropertyItemType;
}

PropertyValueItem* PropertyItem::getValueItem()
{
  return m_ValueItem;
}

void PropertyItem::setValueItem(PropertyValueItem* item, bool deleteItemArgumentIfCopied)
{
  if (item)
  {
    if (m_ValueItem)
    {
      m_ValueItem->setData(item->data(Qt::DisplayRole),
                           Qt::DisplayRole);  // Copy the data to the existing item

      if (deleteItemArgumentIfCopied)
      {
        delete item;
      }
    }
    else if (parent() != NULL)
    {
      // This is a brand new item, so make sure to add it to the model data as well.
      m_ValueItem = item;
      parent()->setChild(row(), 1, m_ValueItem);
    }
  }
}

QString PropertyItem::getFullName()
{
  return m_FullName;
}
