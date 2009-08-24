#include <QtCore>
#include <QtGui>
#include <QtSql>
#include "trackcollection.h"
#include "playlisttablemodel.h"


PlaylistTableModel::PlaylistTableModel(QWidget* parent, TrackCollection* pTrackCollection, int playlistId) : TrackModel(), QSqlRelationalTableModel(parent, pTrackCollection->getDatabase())
{
	m_pTrackCollection = pTrackCollection;
    m_iPlaylistId = playlistId;

	setTable("PlaylistTracks");
	//setFilter("playlist_id = " + QString(m_iPlaylistId));
	
	//Hide columns in the tablemodel that we don't want to show.
	/*
	removeColumn(this->fieldIndex(LIBRARYTABLE_ID)); 
	removeColumn(this->fieldIndex(LIBRARYTABLE_FILENAME));
	removeColumn(this->fieldIndex(LIBRARYTABLE_URL));
	removeColumn(this->fieldIndex(LIBRARYTABLE_LENGTHINBYTES));
	removeColumn(this->fieldIndex(LIBRARYTABLE_CUEPOINT));
	removeColumn(this->fieldIndex(LIBRARYTABLE_WAVESUMMARYHEX));
	removeColumn(this->fieldIndex(LIBRARYTABLE_SAMPLERATE));
	removeColumn(this->fieldIndex(LIBRARYTABLE_CHANNELS));
	removeColumn(this->fieldIndex(LIBRARYTABLE_TRACKNUMBER));


	//Set the column heading labels, rename them for translations and have proper capitalization
    setHeaderData(this->fieldIndex(LIBRARYTABLE_ARTIST), Qt::Horizontal, tr("Artist"));
    setHeaderData(this->fieldIndex(LIBRARYTABLE_TITLE), Qt::Horizontal, tr("Title"));
    setHeaderData(this->fieldIndex(LIBRARYTABLE_ALBUM), Qt::Horizontal, tr("Album"));
    setHeaderData(this->fieldIndex(LIBRARYTABLE_GENRE), Qt::Horizontal, tr("Genre"));
    setHeaderData(this->fieldIndex(LIBRARYTABLE_YEAR), Qt::Horizontal, tr("Year"));            
    setHeaderData(this->fieldIndex(LIBRARYTABLE_LOCATION), Qt::Horizontal, tr("Location"));
    setHeaderData(this->fieldIndex(LIBRARYTABLE_COMMENT), Qt::Horizontal, tr("Comment"));
    setHeaderData(this->fieldIndex(LIBRARYTABLE_DURATION), Qt::Horizontal, tr("Duration"));
    setHeaderData(this->fieldIndex(LIBRARYTABLE_BITRATE), Qt::Horizontal, tr("Bitrate"));
    setHeaderData(this->fieldIndex(LIBRARYTABLE_BPM), Qt::Horizontal, tr("BPM"));
    */
    
   	select(); //Populate the data model.
}

PlaylistTableModel::~PlaylistTableModel()
{
}

void PlaylistTableModel::addTrack(const QModelIndex& index, QString location)
{
	//Note: The model index is ignored when adding to the library track collection.
	//      The position in the library is determined by whatever it's being sorted by,
	//      and there's no arbitrary "unsorted" view.
	m_pTrackCollection->addTrack(location);
	select(); //Repopulate the data model.
}

TrackInfoObject* PlaylistTableModel::getTrack(const QModelIndex& index) const
{
    //FIXME
    /*
	const int positionColumnIndex = this->fieldIndex("position");
	QString location = index.sibling(index.row(), positionColumnIndex).data().toString();
	return m_pTrackCollection->getTrack(location);*/
	
}

QString PlaylistTableModel::getTrackLocation(const QModelIndex& index) const
{
	//FIXME
	
	/*const int locationColumnIndex = this->fieldIndex(LIBRARYTABLE_LOCATION);
	QString location = index.sibling(index.row(), locationColumnIndex).data().toString();
	return location; */
}

void PlaylistTableModel::removeTrack(const QModelIndex& index)
{
    //FIXME
    
	/*const int locationColumnIndex = this->fieldIndex(LIBRARYTABLE_LOCATION);
	QString location = index.sibling(index.row(), locationColumnIndex).data().toString();
	m_pTrackCollection->removeTrack(location);
	select(); //Repopulate the data model.
	*/
}

void PlaylistTableModel::search(const QString& searchText)
{
    //FIXME: Need to keep filtering by playlist_id too
        //SQL is "playlist_id = " + QString(m_iPlaylistId)

	if (searchText == "")
		this->setFilter("");
	else
		this->setFilter("artist LIKE \'%" + searchText + "%\' OR "
						"title  LIKE \'%" + searchText + "%\'");
}

QMimeData* PlaylistTableModel::mimeData(const QModelIndexList &indexes) const {
    QMimeData *mimeData = new QMimeData();
    QList<QUrl> urls;

    foreach (QModelIndex index, indexes) {
        if (index.isValid()) {
            QUrl url(getTrackLocation(index));
            if (!url.isValid())
              qDebug() << "ERROR invalid url\n";
            else
              urls.append(url);
        }
    }
    mimeData->setUrls(urls);
    return mimeData;
}

Qt::ItemFlags PlaylistTableModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);
    if (!index.isValid())
      return Qt::ItemIsEnabled;

	//Enable dragging songs from this data model to elsewhere (like the waveform widget to
	//load a track into a Player).
    defaultFlags |= Qt::ItemIsDragEnabled;

    return defaultFlags;
}
