#include <QtDebug>
#include <QMenu>
#include <QInputDialog>
#include <QFileDialog>
#include <QDesktopServices>
#include <QDateTime>

#include "library/setlogfeature.h"

#include "controlobject.h"
#include "library/playlisttablemodel.h"
#include "library/trackcollection.h"
#include "playerinfo.h"
#include "treeitem.h"

SetlogFeature::SetlogFeature(QObject* parent,
                             ConfigObject<ConfigValue>* pConfig,
                             TrackCollection* pTrackCollection)
        : BasePlaylistFeature(parent, pConfig, pTrackCollection,
                              "SETLOGHOME", "qrc:/html/setlogs.html") {
    m_pPlaylistTableModel = new PlaylistTableModel(this, pTrackCollection,
                                                   "mixxx.db.model.setlog");
    m_pJoinWithPreviousAction = new QAction(tr("Join with previous"), this);
    connect(m_pJoinWithPreviousAction, SIGNAL(triggered()),
            this, SLOT(slotJoinWithPrevious()));

    //create a new playlist for today
    QString set_log_name_format;
    QString set_log_name;

    set_log_name = QDate::currentDate().toString(Qt::ISODate);
    set_log_name_format = set_log_name + " (%1)";
    int i = 1;

    // calculate name of the todays setlog
    while (m_playlistDao.getPlaylistIdFromName(set_log_name) != -1) {
        set_log_name = set_log_name_format.arg(++i);
    }

    m_playlistId = m_playlistDao.createPlaylist(set_log_name,
                                                PlaylistDAO::PLHT_SET_LOG);

    if (m_playlistId == -1) {
        qDebug() << "Playlist Creation Failed";
        qDebug() << "An unknown error occurred while creating playlist: " << set_log_name;
    }

    // Setup the sidebar playlist model
    m_playlistTableModel.setTable("Playlists");
    m_playlistTableModel.setFilter("hidden=2"); // PLHT_SET_LOG
    m_playlistTableModel.setSort(m_playlistTableModel.fieldIndex("id"),
                                 Qt::AscendingOrder);
    m_playlistTableModel.select();

    //construct child model
    TreeItem *rootItem = new TreeItem();
    m_childModel.setRootItem(rootItem);
    constructChildModel(-1);
}

SetlogFeature::~SetlogFeature() {
}

QVariant SetlogFeature::title() {
    return tr("Set Logs");
}

QIcon SetlogFeature::getIcon() {
    return QIcon(":/images/library/ic_library_setlog.png");
}

void SetlogFeature::bindWidget(WLibrarySidebar* sidebarWidget,
                               WLibrary* libraryWidget,
                               MixxxKeyboard* keyboard) {
    BasePlaylistFeature::bindWidget(sidebarWidget,
                                    libraryWidget,
                                    keyboard);
    connect(&PlayerInfo::Instance(), SIGNAL(currentPlayingDeckChanged(int)),
            this, SLOT(slotPlayingDeckChanged(int)));
}

void SetlogFeature::onRightClick(const QPoint& globalPos) {
    Q_UNUSED(globalPos);
    m_lastRightClickedIndex = QModelIndex();

    // Create the right-click menu
    // QMenu menu(NULL);
    // menu.addAction(m_pCreatePlaylistAction);
    // TODO(DASCHUER) add something like disable logging
    // menu.exec(globalPos);
}

void SetlogFeature::onRightClickChild(const QPoint& globalPos, QModelIndex index) {
    //Save the model index so we can get it in the action slots...
    m_lastRightClickedIndex = index;
    QString playlistName = index.data().toString();
    int playlistId = m_playlistDao.getPlaylistIdFromName(playlistName);


    bool locked = m_playlistDao.isPlaylistLocked(playlistId);
    m_pDeletePlaylistAction->setEnabled(!locked);
    m_pRenamePlaylistAction->setEnabled(!locked);

    m_pLockPlaylistAction->setText(locked ? tr("Unlock") : tr("Lock"));


    //Create the right-click menu
    QMenu menu(NULL);
    //menu.addAction(m_pCreatePlaylistAction);
    //menu.addSeparator();
    menu.addAction(m_pAddToAutoDJAction);
    menu.addAction(m_pAddToAutoDJTopAction);
    menu.addAction(m_pRenamePlaylistAction);
    if (playlistId != m_playlistId) {
        // Todays playlist should not be locked or deleted
        menu.addAction(m_pDeletePlaylistAction);
        menu.addAction(m_pLockPlaylistAction);
    }
    if (index.row() > 0) {
        // The very first setlog cannot be joint
        menu.addAction(m_pJoinWithPreviousAction);
    }
    menu.addSeparator();
    menu.addAction(m_pExportPlaylistAction);
    menu.exec(globalPos);
}

bool SetlogFeature::dropAcceptChild(const QModelIndex& index, QUrl url){
    Q_UNUSED(url);
    Q_UNUSED(index);
    return false;
}

bool SetlogFeature::dragMoveAcceptChild(const QModelIndex& index, QUrl url) {
    Q_UNUSED(url);
    Q_UNUSED(index);
    return false;
}

/**
  * Purpose: When inserting or removing playlists,
  * we require the sidebar model not to reset.
  * This method queries the database and does dynamic insertion
*/
QModelIndex SetlogFeature::constructChildModel(int selected_id)
{
    QList<TreeItem*> data_list;
    int nameColumn = m_playlistTableModel.record().indexOf("name");
    int idColumn = m_playlistTableModel.record().indexOf("id");
    int selected_row = -1;
    // Access the invisible root item
    TreeItem* root = m_childModel.getItem(QModelIndex());

    // Create new TreeItems for the playlists in the database
    for (int row = 0; row < m_playlistTableModel.rowCount(); ++row) {
        QModelIndex ind = m_playlistTableModel.index(row, nameColumn);
        QString playlist_name = m_playlistTableModel.data(ind).toString();
        ind = m_playlistTableModel.index(row, idColumn);
        int playlist_id = m_playlistTableModel.data(ind).toInt();

        if ( selected_id == playlist_id) {
            // save index for selection
            selected_row = row;
        }

        // Create the TreeItem whose parent is the invisible root item
        TreeItem* item = new TreeItem(playlist_name, playlist_name, this, root);
        if (playlist_id == m_playlistId) {
            item->setIcon(QIcon(":/images/library/ic_library_setlog_current.png"));
        } else if (m_playlistDao.isPlaylistLocked(playlist_id)) {
            item->setIcon(QIcon(":/images/library/ic_library_locked.png"));
        } else {
            item->setIcon(QIcon());
        }
        data_list.append(item);
    }

    // Append all the newly created TreeItems in a dynamic way to the childmodel
    m_childModel.insertRows(data_list, 0, m_playlistTableModel.rowCount());
    if (selected_row == -1) {
        return QModelIndex();
    }
    return m_childModel.index(selected_row, 0);
}

void SetlogFeature::slotJoinWithPrevious() {
    //qDebug() << "slotJoinWithPrevious() row:" << m_lastRightClickedIndex.data();

    if (m_lastRightClickedIndex.isValid()) {
        int currentPlaylistId = m_playlistDao.getPlaylistIdFromName(
            m_lastRightClickedIndex.data().toString());

        if (currentPlaylistId >= 0) {

            bool locked = m_playlistDao.isPlaylistLocked(currentPlaylistId);

            if (locked) {
                qDebug() << "Skipping playlist deletion because playlist" << currentPlaylistId << "is locked.";
                return;
            }

            // Add every track from right klicked playlist to that with the next smaller ID
            int previousPlaylistId = m_playlistDao.getPreviousPlaylist(currentPlaylistId, PlaylistDAO::PLHT_SET_LOG);
            if (previousPlaylistId >= 0) {

                m_pPlaylistTableModel->setPlaylist(previousPlaylistId);

                if (currentPlaylistId == m_playlistId) {
                    // mark all the Tracks in the previous Playlist as played

                    m_pPlaylistTableModel->select();
                    int rows = m_pPlaylistTableModel->rowCount();
                    for(int i = 0; i < rows; ++i){
                        QModelIndex index = m_pPlaylistTableModel->index(i,0);
                        if (index.isValid()) {
                            TrackPointer track = m_pPlaylistTableModel->getTrack(index);
                            // Do not update the playcount, just set played
                            // status.
                            track->setPlayed(true);
                        }
                    }

                    // Change current setlog
                    m_playlistId = previousPlaylistId;
                }
                qDebug() << "slotJoinWithPrevious() current:" << currentPlaylistId << " previous:" << previousPlaylistId;
                m_playlistDao.copyPlaylistTracks(currentPlaylistId, previousPlaylistId);
                m_playlistDao.deletePlaylist(currentPlaylistId);
                slotPlaylistTableChanged(previousPlaylistId); // For moving selection
                emit(showTrackModel(m_pPlaylistTableModel));
                emit(featureUpdated());
            }
        }
    }
}

void SetlogFeature::slotPlayingDeckChanged(int deck) {
    if (deck > 0) {
        QString chan = QString("[Channel%1]").arg(deck);
        TrackPointer currentPlayingTrack =
                PlayerInfo::Instance().getTrackInfo(chan);
        if (!currentPlayingTrack) {
            return;
        }
        int currentPlayingTrackId = currentPlayingTrack->getId();
        // We can only add tracks that are Mixxx library tracks, not external
        // sources.
        if (currentPlayingTrackId < 0) {
            return;
        }

        // Here the song is realy played, not only loaded.
        currentPlayingTrack->setPlayedAndUpdatePlaycount(true);

        if (m_pPlaylistTableModel->getPlaylist() == m_playlistId) {
            // View needs a refresh
            m_pPlaylistTableModel->appendTrack(currentPlayingTrackId);
        } else {
            m_playlistDao.appendTrackToPlaylist(currentPlayingTrackId,
                                                m_playlistId);
        }
    }
}