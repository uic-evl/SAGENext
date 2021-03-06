#include "system/sn_resourcemonitor.h"
#include "system/sn_resourcemonitorwidget.h"
#include "system/sn_scheduler.h"
#include "system/sn_prioritygrid.h"

#include "sn_scene.h"

//#include "common/sn_commonitem.h"

#include "applications/base/sn_affinityinfo.h"
#include "applications/base/sn_perfmonitor.h"
#include "applications/base/sn_railawarewidget.h"
#include "applications/base/sn_priority.h"

#include <QSettings>

SN_ProcessorNode::SN_ProcessorNode(int type, int i)
    : nodeType(type)
    , id(i)
    , cpuUsage(0.0), bandwidth(0.0)
{
	_appList = new QList<SN_RailawareWidget *>();
	_appList->clear();
}

SN_ProcessorNode::~SN_ProcessorNode() {
	if (_appList) {
		delete _appList;
	}
//	qDebug("ProcessorNode::%s() : proc node %d", __FUNCTION__, id);
}

int SN_ProcessorNode::getNumWidgets() const {
	int result = _appList->size();
	return result;
}

void SN_ProcessorNode::addApp(SN_RailawareWidget *app) {

	_rwlock.lockForWrite();
	if ( _appList && !_appList->contains(app) ) {
//		appList->push_back(app);
//		qDebug("ProcessorNode::%s() : app %llu has added to ProcessorNode %d", __FUNCTION__, app->getWidgetPtr()->getGlobalAppId(), id);

		_appList->push_back(app);
		cpuUsage += app->perfMon()->getCpuUsage();
		bandwidth += app->perfMon()->getCurrBW_Mbps();
	}
	_rwlock.unlock();
}

bool SN_ProcessorNode::removeApp(quint64 appid) {

	bool ret = false;

    if(!_appList) return false;

	_rwlock.lockForWrite();

    SN_RailawareWidget *rw = 0;
    for ( int i=0; i<_appList->size(); ++i) {
        rw = _appList->at(i);

        Q_ASSERT(rw && rw->affInfo());
        if (rw->affInfo() && rw->globalAppId() == appid) {
            _appList->removeAt(i);
            ret = true;
        }
    }

	_rwlock.unlock();

    return ret;
}

bool SN_ProcessorNode::removeApp(SN_RailawareWidget *rw) {

	bool ret = false;

	_rwlock.lockForWrite();

	if ( !_appList || _appList->isEmpty()) {
        ret = false;
	}
    else {

        if (_appList->removeAll(rw)) {
//            qDebug("[%s] SN_ProcessorNode::%s() : app %llu has been removed from proc node %d", qPrintable(QTime::currentTime().toString("hh:mm:ss.zzz")), __FUNCTION__, rw->globalAppId(), id);
			ret = true;
        }
    }

	_rwlock.unlock();
    return ret;
}

void SN_ProcessorNode::refresh() {
    /* iterator over appList and add cpuUsage, bandwidth */
    //	const AffinityInfo *aff = 0;
    const SN_PerfMonitor *perf = 0;
    SN_RailawareWidget *rw = 0;

    // reset
    cpuUsage = 0.0;
    bandwidth = 0.0;

	_rwlock.lockForRead();

    for ( int i=0; i<_appList->size(); ++i ) {
        rw = _appList->at(i);
        if (!rw || !rw->affInfo() || !rw->perfMon()) {
            // remove this app from the List
            _appList->removeAt(i);
            --i;
            continue;
        }
        else {
            perf = rw->perfMon();
            Q_ASSERT(perf);
			qDebug() << "[" << QTime::currentTime().toString("hh:mm:ss.zzz") << "] SN_ProcessorNode::refresh() : widget id" << rw->globalAppId();
            cpuUsage += perf->getCpuUsage(); // Ratio
            bandwidth += perf->getCurrBW_Mbps(); // Mbps
        }
    }

	_rwlock.unlock();
}

int SN_ProcessorNode::prioritySum()  {
	int sum = 0;

	for (int i=0; i<_appList->size(); ++i) {
		SN_RailawareWidget *rw = _appList->at(i);
		if (!rw) continue;

		/*!
		  It assumes _priorityQuantized had been set before calling this function
		  */
		sum += rw->priorityQuantized();
	}

	return sum;
}

void SN_ProcessorNode::printOverhead()  {
//	const AffinityInfo *aff = 0;
	const SN_PerfMonitor *perf = 0;
	SN_RailawareWidget *widget = 0;

	if (_appList->size() == 0) {
		qDebug("ProcessorNode::%s() : There's no app on this processor %d", __FUNCTION__, id);
		return;
	}

	for ( int i=0; i<_appList->size(); ++i ) {

		widget = _appList->at(i);

		if (!widget || !widget->affInfo() || !widget->perfMon()) {
			// remove this app from the List
			_appList->removeAt(i);
			--i;
			continue;
		}
		else {
//			widget = aff->widgetPtr();
			perf = widget->perfMon();

			// currRecvFps, currBandwidth,  currConvDelay, currDrawLatency,currDispFps
			qDebug("ProcessorNode::%s() : Processor %d, app %llu [%.2f fps, %.2f Mbps, %.2f ms, %.2f ms, %.2f fps]", __FUNCTION__
			       , id
			       , widget->globalAppId()
			       , perf->getCurrEffectiveFps()
			       , perf->getCurrBW_Mbps()
			       , perf->getCurrUpdateDelay() * 1000.0
			       , perf->getCurrDrawLatency() * 1000.0
			       , perf->getCurrDispFps()
			       );
		}
	}
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////



SN_SimpleProcNode::SN_SimpleProcNode(int procnum, QObject *parent)
    : QObject(parent)
    , _phyid(-1)
    , _coreid(-1)
    , _procNum(procnum)
    , _cpuUsage(0.0)
    , _netBWusage(0.0)
{

}























////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////






SN_ResourceMonitor::SN_ResourceMonitor(const QSettings *s, SN_TheScene *scene, QObject *parent)
    : QObject(parent)
    , procVec(0)
    , settings(s)
    , _theScene(scene)
    , _schedcontrol(0)
    , _pGrid(0)
    , numaInfo(0)
    , _effective_TR_Mbps(0.0)
    , _current_TR_observed_Mbps(0.0)
    , _TR_estimated_Mbps(0.0)
    , _rMonWidget(0)
    , _printDataFlag(false)
{
//	buildProcVector();
//	Q_ASSERT(procVec);

	buildSimpleProcList();

    qreal totalR = settings->value("system/manualtotalbw", 0).toDouble();
	if (totalR > 0) {
		qDebug() << "SN_ResourceMonitor() : TotalResource is set to" << totalR << "Mbps";
	}

	if (settings->value("system/numnumanodes").toInt() > 1) {
		numaInfo = (Numa_Info *)malloc(sizeof(Numa_Info) * settings->value("system/numnumanodes").toInt());
		for (int i=0; i<settings->value("system/numnumanodes").toInt(); ++i) {
			numaInfo[i].local_node = 0;
			numaInfo[i].numa_hit = 0;
			numaInfo[i].numa_miss = 0;
			numaInfo[i].numa_foreign = 0;
			numaInfo[i].other_node = 0;
			numaInfo[i].hit = 0;
			numaInfo[i].miss = 0;
			numaInfo[i].foreign = 0;
			numaInfo[i].local = 0;
			numaInfo[i].other = 0;
		}
	}

	//
	// write some data to a file
	//
    /*
	char *val = getenv("EXP_DATA_FILE");
	if ( val ) {
		QString filename(val);
		if (!filename.isNull() && !filename.isEmpty()) {

			QString now = QDateTime::currentDateTime().toString("_MM.dd_hh.mm.ss.zzz.CSV");
			filename = filename.append(now);

			qWarning() << "\n=====================================================================================================";
			qWarning() << "EXP_DATA_FILE is defined:" << filename;
			qWarning() << "=====================================================================================================\n";

			_printDataFlag = true;

			_dataFile.setFileName(filename);
			if (!_dataFile.open(QIODevice::WriteOnly)) {
				qDebug() << "SN_ResourceMonitor() : failed to open a file" << _dataFile.fileName();
			}
		}
		else {
			qDebug() << "SN_ResourceMonitor() : wrong file name" << val;
		}
	}
    */

	//
	// temporarily attach writeData button
	//
//	SN_PixmapButton *databutton = new SN_PixmapButton(QPixmap(":/resources/group_data_128.png"), settings->value("gui/iconwidth").toDouble());
//	databutton->setPos(150, 10);
//	_theScene->addItem(databutton);
}

SN_ResourceMonitor::~SN_ResourceMonitor() {

    QObject::disconnect(this, 0, 0, 0); // disconnect everything connected to this

	//	if (procTree) delete procTree;
	if (_rMonWidget) {
		_rMonWidget->close();
        delete _rMonWidget;
	}

	if (_schedcontrol) {
		_schedcontrol->killScheduler();
		delete _schedcontrol;
	}

	if (procVec) {
		SN_ProcessorNode *pn =0;
		for (int i=0; i<procVec->size(); ++i) {
			pn = procVec->at(i);
			if(pn) {
				delete pn;
			}
		}
		delete procVec;
	}

	/* I shouldn't delete numaInfoTextItem here */

	if (numaInfo) free(numaInfo);

	if (_dataFile.isOpen()) {
		_dataFile.flush();
		_dataFile.close();
	}

	qDebug("ResourceMonitor::%s()", __FUNCTION__);
}

void SN_ResourceMonitor::setRMonWidget(ResourceMonitorWidget *rmw) {
    _rMonWidget = rmw;
    if (_rMonWidget ) {
        QObject::connect(this, SIGNAL(dataRefreshed()), _rMonWidget, SLOT(refresh()));
    }
}

void SN_ResourceMonitor::timerEvent(QTimerEvent *) {
    //
    // The priority is computed in here
    //
	refresh(); // update all data

	//
	// save data to a file
	//
	if (_printDataFlag) {
//		printPrelimData();
		printData(&_dataTextOut, true);
	}
}

void SN_ResourceMonitor::addSchedulableWidget(SN_BaseWidget *bw) {
	_widgetListRWlock.lockForWrite();

	if (bw) {
        if (bw->priorityData()) {
            bw->priorityData()->setPriorityGrid(_pGrid);
        }

		// shouldn't allow duplicate item
		if (!widgetList.contains(bw)) {
			widgetList.push_front(bw);
		}
        else {
            qDebug() << "\nSN_ResourceMonitor::addSchedulableWidget() : !! duplicate item in the widget list !!" << bw->globalAppId();
        }

        if (_widgetMap.key(bw, 0) == 0) {
            //qDebug() << "adding " << bw->globalAppId();
            _widgetMap.insert(bw->globalAppId(), bw);
        }
        else {
            qDebug() << "\nSN_ResourceMonitor::addSchedulableWidget() : !! duplicate item in the widget map !!" << bw->globalAppId();
        }
	}

	_widgetListRWlock.unlock();
}

void SN_ResourceMonitor::removeSchedulableWidget(SN_BaseWidget *bw) {
	_widgetListRWlock.lockForWrite();
	if(bw) {
		int numremoved = 0;

//		removeApp(bw); // remove rw from SN_ProcessorNode's appList
		numremoved = widgetList.removeAll(bw);

        // all bw that shares the globalAppId will be deleted
//        qDebug() << "removing " << bw->globalAppId();
		numremoved = _widgetMap.remove(bw->globalAppId());
	}
	_widgetListRWlock.unlock();
}

void SN_ResourceMonitor::buildSimpleProcList() {
	bool ok = false;
	int totalCpu = settings->value("system/numcpus").toInt(&ok);

	_simpleProcList.clear();

	if (!ok) {
		qDebug() << "SN_ResourceMonitor::buildSimpleProcList() : can't retrieve total cpu";
		return;
	}

	SN_SimpleProcNode *spn = 0;
	for (int i=0; i<totalCpu; ++i) {

		spn = new SN_SimpleProcNode(i, this);

		_simpleProcList.push_back(spn);

	}
}

//void ResourceMonitor::rearrangeWidgetMultiMap(BaseWidget *bw, int oldpriority) {
//	rwlock.lockForWrite();

//	// remove
//	RailawareWidget *rw = static_cast<RailawareWidget *>(bw);
//	widgetMultiMap.erase( widgetMultiMap.find(oldpriority, rw) );

//	// insert
//	widgetMultiMap.insert(bw->priority(), rw);

//	rwlock.unlock();
//}

void SN_ResourceMonitor::setScheduler(SN_SchedulerControl *sc) {
    Q_ASSERT(sc);
    _schedcontrol = sc;

    //
    // The signal dataRefreshed() is emitted right before ::refresh() returns.
    // This signal will trigger the schedulerControl to emit readyToSchedule() signal.
    // which will invoke scheduler::doSchedule()
    //
    QObject::connect(this, SIGNAL(dataRefreshed()), _schedcontrol, SIGNAL(readyToSchedule()));

    QObject::connect(_schedcontrol, SIGNAL(schedulerStateChanged(bool)), this, SIGNAL(schedulerStateChanged(bool)));
}

void SN_ResourceMonitor::refresh() {
/*
	//
	// refresh per processor info
	//
	if (procVec) {
		//	QByteArray str(128, '\0');
		SN_ProcessorNode *pn = 0;
		for (int i=0; i<procVec->size(); ++i) {
			pn = procVec->at(i);
			pn->refresh();

			//sprintf(str.data(), "CPU%d: %d\t%.2f\t%.2f\n",pn->getID(), pn->appList()->size(), pn->getBW(), pn->getCpuUsage());
			//infoText.append( QString(str) );

			// dont do below here !!
			//qApp->sendPostedEvents(); // looks like this makes user interaction slow
		}
	}
*/

	//
	// reset data
	//
	for (int i=0; i<_simpleProcList.size(); i++) {
		SN_SimpleProcNode *spn = _simpleProcList.at(i);
		spn->setCpuUsage(0.0);
		spn->setNetBWUsage(0.0);
		spn->setNumWidgets(0);
	}

	_widgetListRWlock.lockForRead();

    qreal current_TR_observed_Mbps = 0.0; // the sum of Rcur

    _TR_estimated_Mbps = 0.0;

    qint64 currentMsec = QDateTime::currentMSecsSinceEpoch();

    bool isTR_tooHigh = false;

	QMap<quint64, SN_BaseWidget *>::const_iterator it;
	for (it=_widgetMap.constBegin(); it!=_widgetMap.constEnd(); it++) {
		SN_BaseWidget *bw = it.value();
		Q_ASSERT(bw);

		SN_PerfMonitor *pm = bw->perfMon();
//		Q_ASSERT(pm);
        if (!pm) continue;


        //
        // Each app updates its own cumulativeByteReceived in its receiving thread.
        // rMonitor triggers them to update performance data based only on the cumulativeByteReceived.
        //
        // The Rcur and Ropt will be updated in this function
        //
        pm->updateDataWithCumulativeByteReceived(currentMsec);


        if (bw->isShowInfo())
            bw->updateInfoTextItem();


        //
        // If any of the application can't even consume what the scheduler demanded then
        // TR is set too high
        //
        if (0 < bw->observedQuality_Dq() && bw->observedQuality_Dq() < 0.95) {
            isTR_tooHigh = true;
        }

        //
        // Aggregate the bandwidth the application is actually achieving at this moment.
        // The sum of Rcur
        //
        current_TR_observed_Mbps += pm->getCurrBW_Mbps();


        //
        // The sum of Ropt
        //
        _TR_estimated_Mbps += pm->getRequiredBW_Mbps();


		//
		// Assumes the _cpuOfMine is continusouly updated by worker thread (affInfo->setCpuOfMine())
		//
		SN_AffinityInfo *ai = bw->affInfo();
//		Q_ASSERT(ai);

        if (ai && ai->cpuOfMine() >= 0 ) {

            SN_SimpleProcNode *spn = _simpleProcList.at(ai->cpuOfMine());
            Q_ASSERT(spn);

            spn->addCpuUsage(pm->getCpuUsage());
            spn->addNetBWUsage(pm->getCurrBW_Mbps()); // Mbps
            spn->addNumWidgets(1);
        }
	}

    //
    // Total resource is set manually
    //
    qreal totalbw = settings->value("system/manualtotalbw", 0).toDouble();
    if ( totalbw != 0 ) {
        _effective_TR_Mbps = totalbw;
    }

    //
    // Total resource will be updated automatically
    //
    else {
        if (isTR_tooHigh) {
//            qDebug() << "TR too high: TRopt" << _TR_estimated_Mbps << "max_TReffective" << _effective_TR_Mbps << "cur_TRcur" << _current_TR_observed_Mbps;
            //
            // the sum of current Rcur would reflect the real TR preciser
            //
            _effective_TR_Mbps = qMax(_effective_TR_Mbps, current_TR_observed_Mbps);
        }
        else {
//            qDebug() << "TRe" << _TR_estimated_Mbps << "max_TRo" << _effective_TR_Mbps << "cur_TRo" << _current_TR_observed_Mbps;
            _effective_TR_Mbps = _TR_estimated_Mbps;
        }
    }

    _current_TR_observed_Mbps = current_TR_observed_Mbps;

	_widgetListRWlock.unlock();

    //
    // This will invoke RMonitorWidget::refresh()
    // This will also invoke SchedulerControl::readyToSchedule() signal which is connected to the scheculer::doSchedule() slot
    //
    emit dataRefreshed();
}

/*
void SN_ResourceMonitor::buildProcVector() {
	int totalCpu = settings->value("system/numcpus").toInt();

	Q_ASSERT(!procVec);
	procVec = new QVector<SN_ProcessorNode *>();

	SN_ProcessorNode *pn = 0;
	for (int i=0; i<totalCpu; ++i) {
		pn = new SN_ProcessorNode(SN_ProcessorNode::PHY_CORE, i);
		if ( settings->value("system/threadpercpu").toInt() > 1 ) {
			pn->setNodeType(SN_ProcessorNode::SMT_CORE);

			  //only works in venom (two intel quad core smt cpus)
			if ( i < 8 ) {
				pn->setSMTSiblingID( i + 8 );
			}
			else {
				pn->setSMTSiblingID(i - 8);
			}

		}
		procVec->push_back(pn);
	}
}
*/

/*
void SN_ResourceMonitor::removeApp(SN_BaseWidget *rw) {
    //	AffinityInfo *aff = static_cast<AffinityInfo *>(obj);
    SN_ProcessorNode *pn = 0;
	if (!procVec) return;
    for ( int i=0; i<procVec->size(); ++i) {
        pn = procVec->at(i);
        if(pn) {
            if ( pn->removeApp(rw)  ) {
                emit appRemoved(i);
//                qDebug("SN_ResourceMonitor::%s() : app %llu has been removed from proc node %d", __FUNCTION__, rw->globalAppId(), i);
            }
        }
    }
}
*/


/*!
  This slot connected to AffinityInfo::cpuOfMineChanged signal in SN_RailawareWidget
  */
void SN_ResourceMonitor::updateAffInfo(SN_RailawareWidget *rw, int oldproc, int newproc) {
	//	affinf->getGlobalAppId();
	//	affinf->getCpuOfMine();

	if ( !procVec ) return;

	SN_ProcessorNode *pn = 0;

	//
	// Note that below code block has Race Condition issue !
	// SN_ProcessorNode sometimes gets to access invalid rw or rw->perfMon
	//

	// O(1)
	if ( oldproc >= 0 && newproc >= 0 ) {
		Q_ASSERT( 0 <= oldproc && oldproc < procVec->size() );
		pn = procVec->at(oldproc);
		Q_ASSERT(pn && oldproc == pn->getID());
		pn->removeApp(rw);
		pn = 0;

		Q_ASSERT( 0 <= newproc && newproc < procVec->size() );
		pn = procVec->at(newproc);
		Q_ASSERT(pn && newproc == pn->getID());
		//qDebug() << "rMonitor::updateAffInfo() : ----------- adding app id " << rw->globalAppId() << " to proc " << pn->getID();
		pn->addApp(rw);
	}
	// for each processor, O(n)
	else {
		for (int i=0; i<procVec->size(); ++i) {
			pn = procVec->at(i);
			Q_ASSERT(pn);

			// if affinityInfo of this widget tells it is affine to processor i then
			if ( i == rw->affInfo()->cpuOfMine() ) {
				Q_ASSERT( i == pn->getID() );
				//qDebug() << "rMonitor::updateAffInfo() : ----------- adding app id " << rw->globalAppId() << " to proc " << pn->getID();
				pn->addApp(rw);
			}
			else {
				pn->removeApp(rw);
			}
		}
	}
}












/*
SN_RailawareWidget * SN_ResourceMonitor::getEarliestDeadlineWidget() {
	_widgetListRWlock.lockForRead();

	qreal temp = -1;
	SN_RailawareWidget *result = 0;
	//	QMultiMap<int, RailawareWidget *>::iterator it;
	QList<SN_RailawareWidget *>::iterator it;
	//	for(it=widgetMultiMap.begin(); it!=widgetMultiMap.end(); it++) {
	for(it=widgetList.begin(); it!=widgetList.end(); it++) {
		//RailawareWidget *r = it.value();
		SN_RailawareWidget *r = *it;
		if ( temp == -1 || temp > r->perfMon()->ts_nextframe() ) {
			temp = r->perfMon()->ts_nextframe();
			result = r;
		}
	}

	_widgetListRWlock.unlock();
	return result;
}
*/



/***
SN_ProcessorNode * SN_ResourceMonitor::getMostUnderloadedProcessor() {
	SN_ProcessorNode *pn = 0;
	SN_ProcessorNode *result = 0;
	qreal temp = INT_MAX;
	for (int i=0; i<procVec->size(); ++i) {
		pn = procVec->at(i);
		pn->refresh();

		if ( pn->getBW() < temp ) {
			temp = pn->getBW();
			result = pn;
		}
	}
	qDebug("%s::%s() : returning pid %d", metaObject()->className(), __FUNCTION__, result->getID());
	return result;
}

int SN_ResourceMonitor::assignProcessor(SN_RailawareWidget *rw, SN_ProcessorNode *pn) {
        if (!rw || !rw->affInfo()) return -1;
        if (!pn) return -1;

        // this will clear ready bit first then set one, followed by set flag
        // because of the flag, SagePixelReceiver::run() will end up calling AffinityInfo::applyNewParameters()

        //        updateAffInfo(rw, -1, pn->getID());

        // Add widget to the processor
        pn->addApp(rw);

        // this will trigger AffinityInfo::applyNewParameters() from SagePixelReceiver
        // which will trigger ResourceMonitor::updateAffInfo() from AffinityInfo
        rw->affInfo()->setReadyBit(AffinityInfo::CPU_MASK, pn->getID());

        return pn->getID();

        // set thread affinity for SageBlockStreamer
        //QByteArray ba(AffinityInfo::Num_Cpus, '0');
        //ba[pn->getID()] = '1';
        //rw->affInfo()->setSageStreamerAffinity(ba.constData());
}

int SN_ResourceMonitor::assignProcessor(SN_BaseWidget *rw) {
	if (!rw || !rw->affInfo()) return -1;

	SN_ProcessorNode *pn = getMostUnderloadedProcessor();
	if (!pn) return -1;

	return assignProcessor(rw, pn);
}

//  Re assign rail for all processes
int SN_ResourceMonitor::assignProcessor() {

	// pause scheduler
	if(_schedcontrol) {
		_schedcontrol->killScheduler();
	}

	foreach (SN_BaseWidget *w, widgetList) {

        if (!w->affInfo()) continue;

		// disconnect signal/slot connection for now
		if (!disconnect(w->affInfo(), SIGNAL(cpuOfMineChanged(SN_RailawareWidget*,int,int)), this, SLOT(updateAffInfo(SN_RailawareWidget*,int,int))) ) {
			qDebug() << "disconnect() failed";
		}

		// remove all widget from pn _appList
		removeApp(w); // what about loadBalance() connection?
	}

	//qApp->sendPostedEvents();

	for (int i=0; i<procVec->size(); ++i) {
		SN_ProcessorNode *pn = procVec->at(i);
		pn->setBW(0);
		pn->setCpuUsage(0);
	}

	//sleep(1);

	QList<SN_BaseWidget *>::iterator it;
	SN_BaseWidget *rw = 0;
	int count = 0;

	for(it=widgetList.begin(); it!=widgetList.end();  it++) {
		rw = *it;
		if ( assignProcessor(rw) >= 0 ) {
			++count;
			//qApp->flush();

			//qApp->sendPostedEvents();
			//QThread::yieldCurrentThread();

			//sleep(1);
			//refresh();
		}
		// perf data shouldn't be reset here
		//rw->perfMon()->reset();

		// reconnect
		connect(rw->affInfo(), SIGNAL(cpuOfMineChanged(SN_RailawareWidget*,int,int)), this, SLOT(updateAffInfo(SN_RailawareWidget*,int,int)));
	}


	// resume scheduler
	if(_schedcontrol) {
		_schedcontrol->launchScheduler(settings->value("system/scheduler_type").toString());
	}

	if (count != widgetList.size()) {
		qDebug("%s::%s() : Error: %d widgets are assigned. widgetList has %d items",metaObject()->className(), __FUNCTION__, count, widgetList.size());
		return -1;
	}
	return count;
}
*/

void SN_ResourceMonitor::resetProcessorAllocation(SN_BaseWidget *rw) {
	if (rw && rw->affInfo())
		rw->affInfo()->setAllReadyBit();
}

void SN_ResourceMonitor::resetProcessorAllocation() {
	QList<SN_BaseWidget *>::iterator it;
	SN_BaseWidget *rw = 0;

	_widgetListRWlock.lockForRead();

	for(it=widgetList.begin(); it!=widgetList.end();  it++) {
		rw = *it;
		Q_ASSERT(rw);
		Q_ASSERT(rw->perfMon());
		rw->perfMon()->reset(); // reset perf data
		resetProcessorAllocation(rw);
	}
	_widgetListRWlock.unlock();
}

/****
SN_ProcessorNode * SN_ResourceMonitor::processor(int pid) {
	if (!procVec) {
		qCritical("%s::%s() : procVec is null", metaObject()->className(), __FUNCTION__);
		return 0;
	}
	return procVec->at(pid);
}


void SN_ResourceMonitor::loadBalance() {
	// find the most overloaded processor
	SN_ProcessorNode *pn_high = 0, *pn_low=0;
	qreal loadhigh = 0.0, loadlow = 100.0;

	if (!procVec || procVec->isEmpty()) return;

	// for each processor
	for (int i=0; i<procVec->size(); ++i) {
		SN_ProcessorNode *tmp = procVec->at(i);

		if (tmp->getCpuUsage() > loadhigh) {
			loadhigh = tmp->getCpuUsage();
			pn_high = tmp;
		}
		else if (tmp->getCpuUsage() < loadlow) {
			loadlow = tmp->getCpuUsage();
			pn_low = tmp;
		}
	}
	qDebug("ResourceMonitor::%s() : H: (%d, %.3f),  L: (%d, %.3f)", __FUNCTION__, pn_high->getID(), loadhigh, pn_low->getID(), loadlow);

	// Is load factor greater than 0.9 ? otherwise no migratino needed.
	if (loadhigh <= 0.9) {
		// do nothing
		// What if pn_low has no widget ?? Isn't it better to migrate to pn_low ?
		return;
	}

	if (pn_high == pn_low) return;

        //
        // Which processor to migrate ?????????
        //

	// for now, find the lowest priority widget from heavily loaded processor
	int priority = INT_MAX;
	SN_RailawareWidget *rw = 0;
	QList<SN_RailawareWidget *> *wlist = pn_high->appList();
	for (int i=0; i<wlist->size(); ++i) {
		SN_RailawareWidget *w = wlist->at(i);
		if (!w) continue;
		if ( w->priority() < priority ) {
			priority = w->priority();
			rw = w;
		}
	}

	if (rw) assignProcessor(rw, pn_low);
}

****/





bool SN_ResourceMonitor::setPrintFile(const QString &filepath) {
    if (!filepath.isNull() && !filepath.isEmpty()) {

//        QString now = QDateTime::currentDateTime().toString("_MM.dd_hh.mm.ss.zzz.CSV");
//        filename = filename.append(now);

        if (_dataFile.isOpen()) {
            closeDataFile();
        }

        QString fname = filepath;
        if (_schedcontrol && _schedcontrol->isRunning()) {
            fname.append("_Sched.csv");
        }
        else {
            fname.append("_Nosched.csv");
        }

        _dataFile.setFileName(fname);

        if (!_dataFile.open(QIODevice::WriteOnly)) {
            qDebug() << "SN_ResourceMonitor::setPrintFile() : failed to open a file" << _dataFile.fileName();
            _printDataFlag = false;
        }
        else {
            _dataTextOut.setDevice(&_dataFile);
            _printDataFlag = true;

            qWarning() << "\n=====================================================================================================";
            qWarning() << "SN_ResourceMonitor::setPrintFile() :" << fname;
            qWarning() << "=====================================================================================================\n";

            return true;
        }
    }
    else {
        qDebug() << "SN_ResourceMonitor::setPrintFile() : wrong file name" << filepath;
    }
    return false;
}


void SN_ResourceMonitor::stopPrintData() {
    _printDataFlag = false;

    _dataTextOut << "# Final Bandwidth\n";

    qint64 now = QDateTime::currentMSecsSinceEpoch();

    SN_BaseWidget *bw = 0;
    QMap<quint64, SN_BaseWidget *>::const_iterator it;
	for (it = _widgetMap.constBegin(); it != _widgetMap.constEnd(); it++) {

        bw = it.value();
        if (!bw) continue;

//        qDebug() << bw->globalAppId() << bw->perfMon()->cumulativeByteReceived() << now - bw->perfMon()->measureStartTime();

        qreal final_Mbps = 8.0f * (qreal)bw->perfMon()->cumulativeByteReceived() / (1000.0f * (now - bw->perfMon()->measureStartTime()));

        _dataTextOut << "#," << bw->globalAppId() << "," << final_Mbps << "\n";

    }



    _dataTextOut.flush();
    _dataTextOut.setDevice(0);

    closeDataFile();
}

void SN_ResourceMonitor::closeDataFile() {
	if (_dataFile.isOpen()) {
        qDebug() << "SN_ResourceMonitor::closeDataFile() : closing the data file" << _dataFile.fileName();
		_dataFile.close();
	}
}


void SN_ResourceMonitor::printData(QTextStream *out, bool widgetIDasColCount /* = false */) {
	if (!_dataFile.exists()) {
		qDebug() << "SN_ResourceMonitor::printData() : _dataFile doesn't exist" << _dataFile.fileName();
		return;
	}
	if (!_dataFile.isOpen()) {
		if (!_dataFile.open(QIODevice::WriteOnly)) {
			qDebug() << "SN_ResourceMonitor::printData() : failed to open a _dataFile" << _dataFile.fileName();
			return;
		}
	}
	if (!_dataFile.isWritable()) {
		qDebug() << "SN_ResourceMonitor::printData() : _dataFile is not writable" << _dataFile.fileName();
		return;
	}

    if (!out) {
        qDebug() << "SN_ResourceMonitor::printData() : null QTextStream";
        return;
    }

    if (! out->device()) {
        qDebug() << "SN_ResourceMonitor::printData() : QTextStream has null device";
        return;
    }

    if (! out->device()->isWritable()) {
        qDebug() << "SN_ResourceMonitor::printData() : QTextStream's device isn't writable";
        return;
    }


    //
    // The header
    //
	static qint64 timestamp = 0;
    if (timestamp == 0) {
        timestamp = QDateTime::currentMSecsSinceEpoch();
        (*out) << "#Data from the rMonitor. Timestamp " << timestamp << " msec, rMon & Sched Freq " << settings->value("system/scheduler_freq").toInt() << "msec \n";
    }


    if (_widgetMap.size() == 0) {
        qDebug() << "SN_ResourceMonitor::printData() : there's no widget";
        return;
    }

    //
    // ex)  < # widgets , ratio overlapped , [globalAppId | priority | curBW | reqBW | Dq [|Intr]] , ... >
    //

    //
	// the wall layout factors
    //
	(*out) << _widgetMap.size();
//	        << "," << _theScene->getRatioEmptySpace()
//	        << "," << _theScene->getRatioOverlapped();

    /*
	QSize avgqwinsize = _theScene->getAvgWinSize().toSize();
	int avgwinsize = avgqwinsize.width() * avgqwinsize.height();

	(*out) << avgwinsize << ",";
    */


	//
	// now individual app
	//
	_widgetListRWlock.lockForRead();

	int colcount = 1; // globalAppId begins with 1 not 0
	QMap<quint64, SN_BaseWidget *>::const_iterator it;
	for (it = _widgetMap.constBegin(); it != _widgetMap.constEnd(); it++) {

        if (widgetIDasColCount) {
            //
            // I need to write the data in the right column
            // it.key() is the globalAppId
            // So each column represents unique app
            //
            int offset = it.key() - colcount; // difference b/w actual globalAppId and the expected value

            for (int i=0; i<offset; i++) {
                (*out) << ",";
                colcount++; // advance column count to synchronize with the next widget's gid in the map
            }
        }

		SN_BaseWidget *rw = it.value();
		Q_ASSERT(rw);
        Q_ASSERT(rw->perfMon());

        /*
		QSize qwinsize = (rw->scale() * rw->boundingRect().size().toSize());
		int winsize = qwinsize.width() * qwinsize.height();

        //
        // data item is going to be
        // priority|windowsize
        //
		(*out) << rw->priority();
		(*out) << "|";
		(*out) << winsize;
        */

        //
        // , priority | curBW | reqBW | D.Q. | isInteracting ,
        //

        (*out) << "," << rw->globalAppId();
        (*out) << "|" << rw->priority();
//        (*out) << "|" << rw->observedQuality_Rq(); // currentBW / requiredBW
        (*out) << "|" << rw->perfMon()->getCurrBW_Mbps();
        (*out) << "|" << rw->perfMon()->getRequiredBW_Mbps();
        (*out) << "|" << rw->demandedQuality();
//        (*out) << rw->perfMon()->getCpuTimeSpent_sec();
        if (rw->perfMon()->isInteracting()) {
            (*out) << "|Intr";
        }


		if (it + 1 == _widgetMap.constEnd()) {
			(*out) << "\n"; // this the last app of the line
		}

		colcount++; // the expected globalAppId of the next item
	}

	_widgetListRWlock.unlock();

	out->flush();
}


void SN_ResourceMonitor::printPrelimDataHeader() {
	QDir::setCurrent(QDir::homePath() + "/.sagenext");
#if QT_VERSION >= 0x040700
	_dataFile.setFileName( QString::number(QDateTime::currentMSecsSinceEpoch()) + ".data" );
#else
	_dataFile.setFileName( QDateTime::currentDateTime().toString("MM.dd.yyyy_hh.mm.ss.zzz") + ".data");
#endif
	if ( ! _dataFile.open(QIODevice::ReadWrite) ) {
		qDebug() << "can't open prelim data file" << _dataFile.fileName();
	}
	QString dataHeader = "";
	if ( _schedcontrol && _schedcontrol->isRunning() ) {
		SN_SelfAdjustingScheduler *sas = static_cast<SN_SelfAdjustingScheduler *>(_schedcontrol->scheduler());
		dataHeader.append("Scheduler (");
		// freq, sensitivity, aggression, greediness
		dataHeader.append("Frequency: " + QString::number(_schedcontrol->scheduler()->frequency()) );
		dataHeader.append(" Sensitivity: " + QString::number(sas->getQTH()));
		dataHeader.append(" Aggression: " + QString::number(sas->getDecF()));
		dataHeader.append(" Greediness: " + QString::number(sas->getIncF()));
		dataHeader.append(")");
	}
	else {
		dataHeader.append("No Scheduler");
	}

	if (settings->value("system/sailaffinity").toBool()) {
		dataHeader.append(", SAIL Affinity");
	}
	if (settings->value("system/threadaffinityonirq").toBool()) {
		dataHeader.append(", Thread Affinity on IRQ");
	}
	dataHeader.append("\n");
	_dataFile.write( qPrintable(dataHeader) );
}

/*!
  This function should be called at every time slice
  */
void SN_ResourceMonitor::printPrelimData() {

//	QString filename_date(filename);
//	filename_date.append( QString::number(QDateTime::currentMSecsSinceEpoch()));
//	QFile file(filename_date);
//	if ( ! file.open(QIODevice::ReadWrite) ) {
//		qCritical() << file << " couldn't be opened for writing";
//		return;
//	}

	if (widgetList.size() == 0 ) return;

	int count = 0;
	_widgetListRWlock.lockForRead();

	// # of apps
	_dataFile.write( qPrintable(QString::number(widgetList.size())) );
	_dataFile.write(",");

	// cpu usage aggregate
	qreal cpuUsageSum = 0.0;
	foreach (SN_BaseWidget *rw, widgetList) {
		cpuUsageSum += rw->perfMon()->getCpuUsage();
	}
	cpuUsageSum *= 100; // convert to percentage
	_dataFile.write( qPrintable(QString::number(cpuUsageSum)) );
	_dataFile.write(",");

	foreach (SN_BaseWidget *rw, widgetList) {
		++count;
		// time slice is implicit (each row = single time slice)

		// Observed quality of each app
		if ( rw->observedQuality_Rq() >= 1 )
			_dataFile.write( "1.0" );
		else
			_dataFile.write( qPrintable(QString::number(rw->observedQuality_Rq())) );

		if (count < widgetList.size())
			_dataFile.write(",");

	}
	_dataFile.write("\n");
	_widgetListRWlock.unlock();

	// file is closed at the calling function
}
