#include "mediabrowser.h"
#include "mediastorage.h"
#include "../sagenextlauncher.h"
#include "../common/commonitem.h"


//QReadWriteLock SN_MediaBrowser::mediaHashRWLock;
//QHash<QString,QPixmap> SN_MediaBrowser::mediaHash;


SN_MediaItem::SN_MediaItem(SAGENext::MEDIA_TYPE mtype, const QString &filename, const QPixmap &pixmap, QGraphicsItem *parent)
    : SN_PixmapButton(parent)
    , _mediaType(mtype)
    , _filename(filename)
{
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);

    //
    // resize() will be called here.
    // for now it's 128 pixel wide
    //
    setPrimaryPixmap(pixmap, 128);

    /*
    _medianameDisplay = new SN_SimpleTextWidget(12, QColor(Qt::white), QColor(Qt::transparent), this);

    QFileInfo f(_filename);
    _medianameDisplay->setText(f.fileName());
    _medianameDisplay->setX(parent->x());
    */

    //
    // because MediaItem should receive mouse event not the thumbnail
    //
//    _setMediaType();
}

void SN_MediaItem::_setMediaType() {

    if (_filename.isEmpty()) return;

    QFileInfo f(_filename);
    if (f.isDir()) {
        _mediaType = SAGENext::MEDIA_TYPE_UNKNOWN;
    }
    else {
        // Use regular expression instead of below....

        // prob not the best way to do this
        if(_filename.endsWith(".pdf")) {
            _mediaType = SAGENext::MEDIA_TYPE_PDF;
        }
        else if(_filename.endsWith(".so")) {
            _mediaType = SAGENext::MEDIA_TYPE_PLUGIN;
        }
        else if(_filename.endsWith(".mp4") || _filename.endsWith(".avi")) {
            _mediaType = SAGENext::MEDIA_TYPE_VIDEO;
        }
        else { // Else it's a picture
            _mediaType = SAGENext::MEDIA_TYPE_IMAGE;
        }
    }
}







SN_MediaBrowser::SN_MediaBrowser(SN_Launcher *launcher, quint64 globalappid, const QSettings *s, SN_MediaStorage* mediaStorage, QGraphicsItem *parent, Qt::WindowFlags wflags)

    : SN_BaseWidget(globalappid, s, parent, wflags)

    , _currItemsDisplayed(0)
    , _launcher(launcher)
    , _mediaStorage(mediaStorage)
    , _rootWindowLayout(0)
{
    QObject::connect(_mediaStorage, SIGNAL(mediaItemClicked(SAGENext::MEDIA_TYPE,QString)), this, SLOT(launchMedia(SAGENext::MEDIA_TYPE,QString)));

    QObject::connect(_mediaStorage, SIGNAL(newMediaAdded()), this, SLOT(mediaStorageHasNewMedia()) );

    setWidgetType(SN_BaseWidget::Widget_GUI);


    _currentDirectory.setPath(QDir::homePath().append("/.sagenext/media"));

    _rootWindowLayout = new QGraphicsLinearLayout(Qt::Vertical);
    _rootWindowLayout->setItemSpacing(0, 10);

    _rootWindowLayout->addItem(_attachRootIconsForMedia());

    _rootWindowLayout->addItem(_attachRootIconsForApps());

    displayRootWindow();

    qDebug("%s::%s() : called.", metaObject()->className(), __FUNCTION__);
}

SN_MediaBrowser::~SN_MediaBrowser() {
    if (_currItemsDisplayed) delete _currItemsDisplayed;
}

QGraphicsLinearLayout* SN_MediaBrowser::_attachRootIconsForMedia() {
    // video
    SN_PixmapButton *videobutton = new SN_PixmapButton(":/resources/video.png", 128, QString(), this);
    QObject::connect(videobutton, SIGNAL(clicked(int)), this, SLOT(videoIconClicked()));

    // image
    SN_PixmapButton *imagebutton = new SN_PixmapButton(":/resources/image.png", 128, QString(), this);
    QObject::connect(imagebutton, SIGNAL(clicked(int)), this, SLOT(imageIconClicked()));

    // pdf
    SN_PixmapButton *pdfbutton = new SN_PixmapButton(":/resources/pdf.png", 128, QString(), this);
    QObject::connect(pdfbutton, SIGNAL(clicked(int)), this, SLOT(pdfIconClicked()));

    // do linear layout or something to arrange them
    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout(Qt::Horizontal);
    layout->setSpacing(16);
    layout->addItem(videobutton);
    layout->addItem(imagebutton);
    layout->addItem(pdfbutton);

    _rootIcons.push_back(videobutton);
    _rootIcons.push_back(imagebutton);
    _rootIcons.push_back(pdfbutton);

    return layout;
}

QGraphicsLinearLayout* SN_MediaBrowser::_attachRootIconsForApps() {
    // get the list of plugins from the SN_Launcher
    // and get the icon for each plugin (the icon has to be provided by plugin)


    // But for now (for the SC12)
    // Attach them manually
    SN_MediaItem* webbrowser = new SN_MediaItem(SAGENext::MEDIA_TYPE_WEBURL, "http://www.evl.uic.edu", QPixmap(":/resources/webkit_128x128.png"), this);

    // clicking an app icon will launch the application directly
    QObject::connect(webbrowser, SIGNAL(clicked(SAGENext::MEDIA_TYPE,QString)), this, SLOT(launchMedia(SAGENext::MEDIA_TYPE,QString)));


    // do the same for
    // google maps, google docs, MandelBrot
    SN_MediaItem* googlemap = new SN_MediaItem(SAGENext::MEDIA_TYPE_WEBURL, "http://maps.google.com", QPixmap(":/resources/webkit_128x128.png"), this);
    QObject::connect(googlemap, SIGNAL(clicked(SAGENext::MEDIA_TYPE,QString)), this, SLOT(launchMedia(SAGENext::MEDIA_TYPE,QString)));


    SN_MediaItem* mandelbrot = new SN_MediaItem(SAGENext::MEDIA_TYPE_PLUGIN, QDir::homePath()+"/.sagenext/media/plugins/libMandelbrotExamplePlugin.so", QPixmap(":/resources/group_data_128.png"),this);
    QObject::connect(mandelbrot, SIGNAL(clicked(SAGENext::MEDIA_TYPE,QString)), this, SLOT(launchMedia(SAGENext::MEDIA_TYPE,QString)));


    // do linear layout or something to arrange them
    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout(Qt::Horizontal);
    layout->addItem(webbrowser);
    layout->addItem(googlemap);
    layout->addItem(mandelbrot);

    _rootIcons.push_back(webbrowser);
    _rootIcons.push_back(googlemap);
    _rootIcons.push_back(mandelbrot);

    return layout;
}


/*
bool SN_MediaBrowser::insertNewMediaToHash(const QString &key, QPixmap &pixmap) {
    if (_mediaStorage) {
        return insertNewMediaToHash(key, pixmap);
    }
    return false;
}
*/


/*
void SN_MediaBrowser::attachItems() {
    attachItemsInCurrDirectory();
    // detach all media items
    foreach (QGraphicsItem *item, childItems()) {
        // cast item both as folder and a media item
        SN_MediaItem *mitem = dynamic_cast<SN_MediaItem *>(item);

        if (mitem) {
            delete mitem;
        }
    }

    QHashIterator<QString, QPixmap> i(_currItemsDisplayed);
    int itemCount = 0;
    while (i.hasNext()) {
        i.next();
        qDebug() << i.key();
        QPixmap pm = SN_MediaBrowser::mediaHash.value(i.key());
        // TODO: Change logic to change it so it changes dir when a folder is clicked vs. thumbnail clicked
        QFileInfo p(i.key());
        if(p.isFile()) {
            SN_MediaItem *item = new SN_MediaItem(i.key(), pm, this);

            if ( ! QObject::connect(item, SIGNAL(thumbnailClicked(SAGENext::MEDIA_TYPE,QString)), this, SLOT(launchMedia(SAGENext::MEDIA_TYPE,QString))) ) {
                qCritical("\n%s::%s() : signal thumbnailClicked is not connected to the slot launchMedia\n", metaObject()->className(), __FUNCTION__);
            }

            // TODO: The xPos of each thumbnail is hardcoded...MUST CHANGE THIS
            if(item->getMediaType() == SAGENext::MEDIA_TYPE_IMAGE) {
                itemCount++;
                item->moveBy(_settings->value("gui/mediathumbnailwidth",256).toInt() * itemCount, 0);
            }
            else if (item->getMediaType() == SAGENext::MEDIA_TYPE_PDF) {
                itemCount++;
                item->moveBy(_settings->value("gui/mediathumbnailwidth",256).toInt() * itemCount, 0);
            }
            else if(item->getMediaType() == SAGENext::MEDIA_TYPE_PLUGIN) {
                itemCount++;
                item->moveBy(_settings->value("gui/mediathumbnailwidth",256).toInt() * itemCount, 0);
            }
            else if(item->getMediaType() == SAGENext::MEDIA_TYPE_LOCAL_VIDEO){
                itemCount++;
                item->moveBy(_settings->value("gui/mediathumbnailwidth",256).toInt() * itemCount, 0);
            }
        }
        else if(p.isDir()) {
            FolderItem *item = new FolderItem(i.key(), pm, this);

            if ( ! QObject::connect(item, SIGNAL(folderClicked(QString)), this, SLOT(changeDirectory(QString))) ) {
                qCritical("\n%s::%s() : signal thumbnailClicked is not connected to the slot launchMedia\n", metaObject()->className(), __FUNCTION__);
            }
            // Logic here for folder count
            itemCount++;
            item->moveBy(pm.size().width() * itemCount,0);
        }
    }
    */

/*
    SN_MediaBrowser::mediaHashRWLock.lockForRead();
    mediaHash = _mediaStorage->getMediaHashForRead();
    QHashIterator<QString, QPixmap> i(mediaHash);
    int itemCount = 0;
    int itemCount = 0;
    int itemCount = 0;
    int itemCount = 0;
    while (i.hasNext()) {
        i.next();
        QPixmap pm = SN_MediaBrowser::mediaHash.value(i.key());

        MediaItem *item = new MediaItem(i.key(), pm, this);

        if ( ! QObject::connect(item, SIGNAL(thumbnailClicked(SAGENext::MEDIA_TYPE,QString)), this, SLOT(launchMedia(SAGENext::MEDIA_TYPE,QString))) ) {
            qCritical("\n%s::%s() : signal thumbnailClicked is not connected to the slot launchMedia\n", metaObject()->className(), __FUNCTION__);
        }

        // TODO: The xPos of each thumbnail is hardcoded...MUST CHANGE THIS
        if(item->getMediaType() == SAGENext::MEDIA_TYPE_IMAGE) {
            itemCount++;
            item->moveBy(_settings->value("gui/mediathumbnailwidth",256).toInt() * itemCount, 0);
        }
        else if (item->getMediaType() == SAGENext::MEDIA_TYPE_PDF) {
            itemCount++;
            item->moveBy(_settings->value("gui/mediathumbnailwidth",256).toInt() * itemCount, 64);
        }
        else if(item->getMediaType() == SAGENext::MEDIA_TYPE_PLUGIN) {
            itemCount++;
            item->moveBy(_settings->value("gui/mediathumbnailwidth",256).toInt() * itemCount, 128);
        }
        else {
            itemCount++;
            item->moveBy(_settings->value("gui/mediathumbnailwidth",256).toInt() * itemCount, 192);
        }
    }

//    foreach (QFileInfo fileinfo, _currentDirectory.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDot)) {

//        if (fileinfo.isDir()) {
//            // directory icon
//        }
//        else if (fileinfo.isFile()) {
//            QPixmap pm = MediaBrowser::mediaHash.value(fileinfo.absoluteFilePath());
//            MediaItem *item = new MediaItem(fileinfo.absoluteFilePath(), pm.scaled(256, 256), this);
////            item->moveBy(x, y);
//        }

//    }

    SN_MediaBrowser::mediaHashRWLock.unlock();
*/

//}

/*!
  Slot called when MediaStorage has new media.
  MediaBrowser will then update itself to account for new media.
  TODO: Specify by user if there are multiple MediaBrowsers?
 */
void SN_MediaBrowser::mediaStorageHasNewMedia(){
    qDebug() << "MediaBrowser has been informed of changes to MediaStorage";
}

void SN_MediaBrowser::launchMedia(SAGENext::MEDIA_TYPE mtype, const QString &filename) {
    Q_ASSERT(_launcher);

    //
    // this is folder
    //
    if (mtype == SAGENext::MEDIA_TYPE_UNKNOWN) {

        //
        // call changeDirectory() with new directory
        //

        return;
    }

    //
    // Where the widget should be opened ?
    //
    _launcher->launch(mtype, filename /* , scenePos */);
}

void SN_MediaBrowser::displayRootWindow() {

    // delete the list of currently displayed
    // actual items are not deleted.
    if (_currItemsDisplayed) {
        delete _currItemsDisplayed;
        _currItemsDisplayed = 0;
    }

    //
    // show all the root icons
    //
    foreach(SN_PixmapButton* icon, _rootIcons) {
        icon->show();
    }

    // unset the rootWindowLayout
    setLayout(_rootWindowLayout);

    adjustSize();
}

void SN_MediaBrowser::changeDirectory(const QString &dir) {
    _currentDirectory.setPath(dir);

    qDebug() << "SN_MediaBrowser::changeDirectory() " << dir;

    SN_MediaStorage::MediaListRWLock.lockForRead();

    //
    // sets the current list of items to be displayed
    //
    QList<SN_MediaItem *> * itemsInCurrDir = _mediaStorage->getMediaListInDir(_currentDirectory);
    if (itemsInCurrDir && ! itemsInCurrDir->empty()) {

        if (_currItemsDisplayed) {
            delete _currItemsDisplayed;
        }
        _currItemsDisplayed = itemsInCurrDir;
        qDebug() << "SN_MediaBrowser::changeDirectory() : " << _currItemsDisplayed->size() << "items in" << dir;
    }

    SN_MediaStorage::MediaListRWLock.unlock();


    //
    // Hide all the root icons
    //
    foreach(SN_PixmapButton* icon, _rootIcons)
        icon->hide();

    // unset the rootWindowLayout
    setLayout(0);

    //
    // Given the # of items and the size of the thumbnail
    // determine the size of the panel.
    //
    // Organize items in a grid
    //

    // Also attach an icon (to go back to the rootWindow) on the left

    QGraphicsGridLayout *gridlayout = new QGraphicsGridLayout;

    if (_currItemsDisplayed && !_currItemsDisplayed->empty()) {
        foreach(SN_MediaItem *item, *_currItemsDisplayed) {
            //        gridlayout->addItem(item);
        }
    }
    setLayout(gridlayout);
    adjustSize();
}
