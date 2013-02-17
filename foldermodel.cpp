/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2012  <copyright holder> <email>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "foldermodel.h"
#include "icontheme.h"
#include <iostream>
#include <QtAlgorithms>
#include <QVector>

using namespace Fm;

FolderModel::FolderModel() : 
  folder(NULL)  {
/*
    ColumnIcon,
    ColumnName,
    ColumnFileType,
    ColumnMTime,
    NumOfColumns
*/
  }

FolderModel::~FolderModel() {
  if(folder)
    setFolder(NULL);
}

void FolderModel::setFolder(FmFolder* new_folder) {
  if(folder) {
    removeAll();	// remove old items
    g_signal_handlers_disconnect_by_func(folder, onStartLoading, this);
    g_signal_handlers_disconnect_by_func(folder, onFinishLoading, this);
    g_signal_handlers_disconnect_by_func(folder, onFilesAdded, this);
    g_signal_handlers_disconnect_by_func(folder, onFilesChanged, this);
    g_signal_handlers_disconnect_by_func(folder, onFilesRemoved, this);
    g_object_unref(folder);
  }
  if(new_folder) {
    folder = FM_FOLDER(g_object_ref(new_folder));
    g_signal_connect(folder, "start-loading", G_CALLBACK(onStartLoading), this);
    g_signal_connect(folder, "finish-loading", G_CALLBACK(onFinishLoading), this);
    g_signal_connect(folder, "files-added", G_CALLBACK(onFilesAdded), this);
    g_signal_connect(folder, "files-changed", G_CALLBACK(onFilesChanged), this);
    g_signal_connect(folder, "files-removed", G_CALLBACK(onFilesRemoved), this);
    // handle the case if the folder is already loaded
    if(fm_folder_is_loaded(folder))
      insertFiles(0, fm_folder_get_files(folder));
  }
  else
    folder = NULL;
}

void FolderModel::onStartLoading(FmFolder* folder, gpointer user_data) {
  FolderModel* model = static_cast<FolderModel*>(user_data);
  // remove all items
  model->removeAll();
}

void FolderModel::onFinishLoading(FmFolder* folder, gpointer user_data) {
  FolderModel* model = static_cast<FolderModel*>(user_data);
}

void FolderModel::onFilesAdded(FmFolder* folder, GSList* files, gpointer user_data) {
  FolderModel* model = static_cast<FolderModel*>(user_data);
  int n_files = g_slist_length(files);
  model->beginInsertRows(QModelIndex(), model->items.count(), model->items.count() + n_files - 1);
  for(GSList* l = files; l; l = l->next) {
    FmFileInfo* info = FM_FILE_INFO(l->data);
    Item item(info);
/*
    if(fm_file_info_is_hidden(info)) {
      model->hiddenItems.append(item);
      continue;
    }
*/
    model->items.append(item);
  }
  model->endInsertRows();
}

static void FolderModel::onFilesChanged(FmFolder* folder, GSList* files, gpointer user_data) {
  FolderModel* model = static_cast<FolderModel*>(user_data);
  int n_files = g_slist_length(files);
//  model->beginInsertRows(QModelIndex(), model->items.count(), model->items.count() + n_files - 1);
  for(GSList* l = files; l; l = l->next) {
    // Item item(FM_FILE_INFO(l->data));
  }
//  model->endInsertRows();
}

static void FolderModel::onFilesRemoved(FmFolder* folder, GSList* files, gpointer user_data) {
  FolderModel* model = static_cast<FolderModel*>(user_data);
  for(GSList* l = files; l; l = l->next) {
    FmFileInfo* info = FM_FILE_INFO(l->data);
    const char* name = fm_file_info_get_name(info);
    int row;
    QList<Item>::iterator it = model->findItemByName(name, &row);
    if(it != model->items.end()) {
      model->beginRemoveRows(QModelIndex(), row, row);
      model->items.erase(it);
      model->endRemoveRows();
    }
  }
}

void FolderModel::insertFiles(int row, FmFileInfoList* files) {
  int n_files = fm_file_info_list_get_length(files);
  beginInsertRows(QModelIndex(), row, row + n_files - 1);
  for(GList* l = fm_file_info_list_peek_head_link(files); l; l = l->next) {
    Item item(FM_FILE_INFO(l->data));
    items.append(item);
  }
  endInsertRows();
}

void FolderModel::removeAll() {
  if(items.empty())
    return;
  beginRemoveRows(QModelIndex(), 0, items.size() - 1);
  items.clear();
  endRemoveRows();
}

int FolderModel::rowCount(const QModelIndex & parent) const {
  if(parent.isValid())
    return 0;
  return items.size();
}

int FolderModel::columnCount (const QModelIndex & parent = QModelIndex()) const {
  if(parent.isValid())
    return 0;
  return NumOfColumns;
}

FolderModel::Item* FolderModel::itemFromIndex(const QModelIndex& index) const {
  return reinterpret_cast<Item*>(index.internalPointer());
}

FmFileInfo* FolderModel::fileInfoFromIndex(const QModelIndex& index) const {
  Item* item = itemFromIndex(index);
  return item ? item->info : NULL;
}

QVariant FolderModel::data(const QModelIndex & index, int role = Qt::DisplayRole) const {
  if(!index.isValid() || index.row() > items.size() || index.column() >= NumOfColumns) {
    return QVariant();
  }
  Item* item = itemFromIndex(index);
  FmFileInfo* info = item->info;

  switch(role) {
    case Qt::ToolTipRole:
      //break;
    case Qt::DisplayRole:
    {
      switch(index.column()) {
	case ColumnName: {
	  return QVariant(item->displayName);
	}
	case ColumnFileType: {
	  FmMimeType* mime = fm_file_info_get_mime_type(info);
	  const char* desc = fm_mime_type_get_desc(mime);
	  QString str = QString::fromUtf8(desc);
	  return QVariant(str);
	}
	case ColumnMTime: {
	  const char* name = fm_file_info_get_disp_mtime(info);
	  QString str = QString::fromUtf8(name);
	  return QVariant(str);
	}
      }
    }
    case Qt::DecorationRole: {
      if(index.column() == 0) {
	// QPixmap pix = IconTheme::loadIcon(fm_file_info_get_icon(info), iconSize_);
	return QVariant(item->icon);
	// return QVariant(pix);
      }
    }
    case FileInfoRole:
      return qVariantFromValue((void*)info);
  }
  return QVariant();
}

QVariant FolderModel::headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const {
  if(role == Qt::DisplayRole) {
    if(orientation == Qt::Horizontal) {
      QString title;
      switch(section) {
	case ColumnName:
	  title = tr("Name");
	  break;
	case ColumnFileType:
	  title = tr("Type");
	  break;
	case ColumnMTime:
	  title = tr("Modified");
	  break;
      }
      return QVariant(title);
    }
  }
  return QVariant();
}

QModelIndex FolderModel::index(int row, int column, const QModelIndex & parent) const {
  if(row <0 || row >= items.size() || column < 0 || column >= NumOfColumns)
    return QModelIndex();
  const Item& item = items.at(row);
  return createIndex(row, column, (void*)&item);
}

QModelIndex FolderModel::parent(const QModelIndex & index) const {
  return QModelIndex();
}

// FIXME: this is very inefficient and should be replaced with a 
// more reasonable implementation later.
QList<FolderModel::Item>::iterator FolderModel::findItemByPath(FmPath* path, int* row) {
  QList<Item>::iterator it = items.begin();
  int i = 0;
  while(it != items.end()) {
    Item& item = *it;
    FmPath* item_path = fm_file_info_get_path(item.info);
    if(fm_path_equal(item_path, path)) {
      *row = i;
      return it;
    }
    ++it;
    ++i;
  }
  return items.end();
}

// FIXME: this is very inefficient and should be replaced with a 
// more reasonable implementation later.
QList<FolderModel::Item>::iterator FolderModel::findItemByName(const char* name, int* row) {
  QList<Item>::iterator it = items.begin();
  int i = 0;
  while(it != items.end()) {
    Item& item = *it;
    const char* item_name = fm_file_info_get_name(item.info);
    if(strcmp(name, item_name) == 0) {
      *row = i;
      return it;
    }
    ++it;
    ++i;
  }
  return items.end();
}

QStringList FolderModel::mimeTypes() const {
  return QAbstractItemModel::mimeTypes();
}

QMimeData* FolderModel::mimeData(const QModelIndexList& indexes) const {
  return QAbstractItemModel::mimeData(indexes);
}


#if 0
// we do the sorting in ProxyFolderModel instead.

bool FolderModel::Sorter::operator()(const Item &t1, const Item &t2) const {
  // FIXME: using a switch here is inefficient.
  // Having difffernt sorter function objects for different sort column is better.
  bool less = false;
  // dir before files
  bool isdir1 = fm_file_info_is_dir(t1.info);
  bool isdir2 = fm_file_info_is_dir(t2.info);
  if(isdir1 != isdir2) {
    return isdir1 ? true : false;
  }

  switch(model_->sortColumn) {
    case ColumnName:
      less = t1.displayName < t2.displayName;
      break;
    case ColumnMTime:
      less = fm_file_info_get_mtime(t1.info) < fm_file_info_get_mtime(t2.info);
      break;
    case ColumnFileType:
      break;
  };
  return model_->sortOrder == Qt::AscendingOrder ? less : !less;
}

void FolderModel::sort(int column, Qt::SortOrder order = Qt::AscendingOrder) {
//  if(column != ColumnName)
//    return;
  sortColumn = column;
  sortOrder = order;

  Q_EMIT layoutAboutToBeChanged();

  // store old position of each row
  QVector<FmFileInfo*> old_rows(items.count());
  QList<Item>::iterator it;
  int row;
  for(row = 0, it = items.begin(); it != items.end(); ++it, ++row) {
    Item& item = *it;
    old_rows[row] = item.info;
  }

  // sort the real data
  qStableSort(items.begin(), items.end(), Sorter(this));

  // create mapping
  QModelIndexList from_indices, to_indices;
  for(row = 0, it = items.begin(); it != items.end(); ++it, ++row) {
    Item& item = *it;
    int old_row = old_rows.indexOf(item.info);
    to_indices.append(createIndex(row, 0, item.info));
    from_indices.append(createIndex(old_row, 0, item.info));
  }
  changePersistentIndexList(from_indices, to_indices);
  
  Q_EMIT layoutChanged();
}
#endif
