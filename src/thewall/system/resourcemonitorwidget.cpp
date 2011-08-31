#include "resourcemonitorwidget.h"
#include "resourcemonitor.h"
#include "sagenextscheduler.h"
#include "ui_resourcemonitorwidget.h"

#include "../graphicsviewmainwindow.h"

#include "../applications/base/railawarewidget.h"
//#include "../applications/sagestreamwidget.h"
#include "../applications/base/perfmonitor.h"


ResourceMonitorWidget::ResourceMonitorWidget(ResourceMonitor *rm, SchedulerControl *sc, QWidget *parent) :
	QWidget(parent),
	ui(new Ui::ResourceMonitorWidget),
	rMonitor(rm),
	schedcontrol(sc),
	_numWidgets(0),
	isAllocationEnabled(false),
	isScheduleEnabled(false)
{
	ui->setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);
//	pidList.clear();

	refreshCount = 0;

//	if (schedcontrol) {
//		if (schedcontrol->isRunning()) {
//			ui->scheduleButton->setText("Stop Scheduling");
//			isScheduleEnabled = true;
//		}
//		else {
//			ui->scheduleButton->setText("Run Scheduling");
//			isScheduleEnabled = false;
//		}
//	}

	buildPerCpuHLayouts();

//	layoutButtons();

//	ui->vLayoutOnTheLeft->addStretch();



	/****
	  per widget data table
	 *****/
	ui->perAppPerfTable->setColumnCount(6); // app id,  priority, cpu usage, curr recv FPS, curr quality, desired quality
	ui->perAppPerfTable->setRowCount(0);
	QStringList headers;
	headers << "Id" << "Priority" << "Curr Recv FPS" << "CPU usage" << "Observed Quality" << "Adjusted Quality";
	ui->perAppPerfTable->setHorizontalHeaderLabels(headers);



	ui->perfPerPriorityRankTable->setColumnCount(5); // rank, recvfps, observed quality, cpu usage, count
	ui->perfPerPriorityRankTable->setRowCount(0);
	QStringList headers2;
	headers2 << "Rank" << "Avg FPS" << "Avg Quality" << "Avg CPU" << "count";
	ui->perfPerPriorityRankTable->setHorizontalHeaderLabels(headers2);


	/* plot */
	/*
	plot = new SchedulingPlot(rm, schedcontrol->scheduler(), this);
	(void) new CanvasPicker(plot);
	plot->setFiducialMarkerPos(QPoint(0, 1));

	ui->tablePlotVLayout->addWidget(plot);
	ui->tablePlotVLayout->setStretchFactor(ui->tableWidget, 2);
	ui->tablePlotVLayout->setStretchFactor(plot, 1);
	*/

}

ResourceMonitorWidget::~ResourceMonitorWidget() {
	if (ui) delete ui;
//	if (plot) delete plot;
	qDebug("%s::%s()", metaObject()->className(), __FUNCTION__);
}

void ResourceMonitorWidget::refresh() {

	++refreshCount;

	/***
   Scheduler Parameter
   ***/
	if (schedcontrol && schedcontrol->scheduler()) {
		ui->label_schedparam_freq->setNum(schedcontrol->scheduler()->frequency());
		SelfAdjustingScheduler *sas = qobject_cast<SelfAdjustingScheduler *>(schedcontrol->scheduler());
		if (sas) {
			ui->label_schedparam_sens->setNum( sas->getQTH() );
			ui->label_schedparam_greed->setNum( sas->getIncF() );
			ui->label_schedparam_aggr->setNum( sas->getDecF() );
		}
	}

	/*****
	  per CPU bandwidth and load
	  ****/
	QVector<ProcessorNode *> *pv = rMonitor->getProcVec();
	foreach(ProcessorNode *pn , *pv) {
		QLayoutItem *li = ui->vLayout_percpu->itemAt(1 + pn->getID()); // offset with 1 to skip header hlayout
		QLayout *l = li->layout(); // this is HBoxLayout for each row
		QLabel *lb = 0;

		lb = qobject_cast<QLabel *>(l->itemAt(2)->widget());
		lb->setNum(pn->getNumWidgets());

		lb = qobject_cast<QLabel *>(l->itemAt(3)->widget());
		lb->setNum(pn->getBW());

		lb = qobject_cast<QLabel *>(l->itemAt(4)->widget());
		lb->setNum(pn->getCpuUsage());
	}

	/***************
	  per widget performance data table
	  *************/
	qint64 ctse = QDateTime::currentMSecsSinceEpoch();
	ui->perAppPerfTable->clearContents(); // because I want to see real time perf info. Data for removed app will be gone.
	ui->perAppPerfTable->setRowCount(rMonitor->getWidgetList().size());
	int currentRow = 0;

	// An widget per row
	foreach(RailawareWidget *rw, rMonitor->getWidgetList()) {
		if (!rw) continue;

		// fill data for each column on current row
		for (int i=0; i<ui->perAppPerfTable->columnCount(); ++i) {
			QTableWidgetItem *item = ui->perAppPerfTable->item(currentRow, i);
			if (!item) {
				item = new QTableWidgetItem;
				// Sets the current row with tableWidgetItem
				// tablewidget takes ownership of an item
				ui->perAppPerfTable->setItem(currentRow, i, item);
			}
			switch(i) {
			case 0:
				item->setData(Qt::DisplayRole, rw->globalAppId());
				break;
			case 1:
				item->setData(Qt::DisplayRole, (int)(100 * rw->priority(ctse)));
				break;
			case 2:
				item->setData(Qt::DisplayRole, rw->perfMon()->getCurrRecvFps());
				break;
			case 3:
				item->setData(Qt::DisplayRole, rw->perfMon()->getCpuUsage());
				break;
			case 4:
				item->setData(Qt::DisplayRole, rw->observedQuality());
				break;
			case 5:
				item->setData(Qt::DisplayRole, rw->desiredQuality());
				break;
//			case 6:
//				//item->setData(Qt::DisplayRole, rw->perfMon()->getRecvFpsStdDeviation());
//				item->setData(Qt::DisplayRole, rw->perfMon()->getAdjustedFps());
//				break;
//			case 7:
//				item->setData(Qt::DisplayRole, rw->failToSchedule);
//				break;
			default:
				break;
			}
		}
		currentRow++;
	}
	ui->perAppPerfTable->sortItems(1, Qt::DescendingOrder);
	ui->perAppPerfTable->resizeColumnsToContents();





	// must be called only after perAppPerfTable has sorted
	/***
	populatePerfDataPerPriorityRank();

	ui->perfPerPriorityRankTable->setRowCount(PerfPerRankMap.size());
	for (int i=0; i<PerfPerRankMap.size(); i++ ) {
		PerfItem pi = PerfPerRankMap.value(i);

		// fill the columns
		for (int col=0; col<ui->perfPerPriorityRankTable->columnCount(); col++) {
			QTableWidgetItem *item = ui->perfPerPriorityRankTable->item(i, col);
			if (!item) {
				item = new QTableWidgetItem;
				ui->perfPerPriorityRankTable->setItem(i, col, item);
			}

			if (col == 0) { // rank
				item->setData(Qt::DisplayRole, i);
			}
			else if (col == 1) { // fps
				item->setData(Qt::DisplayRole, pi.recvfps / (qreal)(pi.count));
			}
			else if (col == 2) { // quality
				item->setData(Qt::DisplayRole, pi.observedquality / (qreal)(pi.count));
			}
			else if (col == 3) { // cpu
				item->setData(Qt::DisplayRole, pi.cpuusage / (qreal)(pi.count));
			}
			else if (col == 4) { // count
				item->setData(Qt::DisplayRole, pi.count);
			}
		}
	}
	***/


	/*******
	  Plot to see data and set fiducial widget
	  ********/
//	plot->updateData(ctse);
}

void ResourceMonitorWidget::populatePerfDataPerPriorityRank() {
	// Table must be sorted by priority already
	// for each widget at this moment
	for (int i=0; i<ui->perAppPerfTable->rowCount(); i++) {
//		int priority = ui->perAppPerfTable->item(i, 1)->data(Qt::DisplayRole).toInt();
//		qDebug() << priority;
		qreal recvfps = ui->perAppPerfTable->item(i, 2)->data(Qt::DisplayRole).toDouble();
		qreal cpuusage = ui->perAppPerfTable->item(i, 3)->data(Qt::DisplayRole).toDouble();
		qreal observedquality = ui->perAppPerfTable->item(i, 4)->data(Qt::DisplayRole).toDouble();

		PerfItem pitem;

		// item exist
		if ( PerfPerRankMap.find(i) != PerfPerRankMap.end() ) {
			pitem = PerfPerRankMap.value(i);
			pitem.recvfps += recvfps;
			pitem.cpuusage += cpuusage;
			pitem.observedquality += observedquality;
			pitem.count += 1;
		}
		else {
			pitem.recvfps = recvfps;
			pitem.cpuusage = cpuusage;
			pitem.observedquality = observedquality;
			pitem.count = 1;
		}
		PerfPerRankMap.insert(i, pitem);
	}
//	qDebug() << "PerfPerRankMap size" << PerfPerRankMap.size();
}

void ResourceMonitorWidget::buildPerCpuHLayouts() {
	QVector<ProcessorNode *> *pv = rMonitor->getProcVec();
//	int numproc = pv->size();

	/* header label */
	QHBoxLayout *hl = new QHBoxLayout();
	hl->addSpacerItem(new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Minimum));
	hl->addWidget(new QLabel("CPU id")); // id
	hl->addWidget(new QLabel("# threads"), 0, Qt::AlignRight); // thread per cpu
	hl->addWidget(new QLabel("BW(Mbps)"), 0, Qt::AlignRight); // bandwidth
	hl->addWidget(new QLabel("CPU Usage"), 0, Qt::AlignRight); // cpu usage

	ui->vLayout_percpu->addLayout(hl);

	/* data */
	foreach(ProcessorNode *pn , *pv) {
		QHBoxLayout *hl = new QHBoxLayout();

		hl->addSpacerItem(new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Minimum));
		hl->addWidget(new QLabel( QString::number(pn->getID()).prepend("CPU") ) ); // id
		hl->addWidget(new QLabel(), 0, Qt::AlignRight); // thread per cpu
		hl->addWidget(new QLabel(), 0, Qt::AlignRight); // bandwidth
		hl->addWidget(new QLabel(), 0, Qt::AlignRight); // cpu usage

		ui->vLayout_percpu->addLayout(hl);
	}
}


//void ResourceMonitorWidget::layoutButtons() {
//	QPushButton *b1 = new QPushButton("Layout 1");
//	QPushButton *b2 = new QPushButton("Layout 2");
//	QPushButton *b3 = new QPushButton("Layout 3");
//	connect(b1, SIGNAL(clicked()), gvmain, SLOT(newLayout1()));
//	connect(b2, SIGNAL(clicked()), gvmain, SLOT(newLayout2()));
//	connect(b3, SIGNAL(clicked()), gvmain, SLOT(newLayout3()));

//	QHBoxLayout *hl = new QHBoxLayout();
//	hl->addWidget(b1);
//	hl->addWidget(b2);
//	hl->addWidget(b3);

//	ui->verticalLayoutOnTheLeft->addLayout(hl);
//}


/*
void SungwonThesis::on_loadTestSetButton_clicked()
{
	if (_mediaFilename.isNull()) {

		// open filedialog (modal)
		_mediaFilename = QFileDialog::getOpenFileName(this, "Open video", QDir::homePath() + "/media/video", "Videos (*.mov *.avi *.mpg *.mp4);;Any (*)", 0, QFileDialog::ReadOnly);
	}

	if (_mediaFilename.isNull()) return;

	QStringList args;
	args << "-vo" << "sage" << "-nosound" << "-loop" << "0" << "-quiet" << _mediaFilename;

	for (int i=0; i<_numWidgets; ++i) {

		QProcess *proc = new QProcess(this);
		proc->start("mplayer",  args); // this will trigger gviewmain::startSageApp()

//		qint64 pid = 0;
//		if ( QProcess::startDetached("mplayer", args, ".", &pid) ) {
//			pidList.push_back(pid);
//		}
//		else {
//			qDebug("SungwonThesis::%s() : failed to start a process", __FUNCTION__);
//		}

		qApp->flush();
		qApp->sendPostedEvents();

	}
}
*/


//void ResourceMonitorWidget::on_allocationButton_clicked()
//{
//	if ( ! isAllocationEnabled ) {
//		// foreach widget, assign a processor
//		int count = rMonitor->assignProcessor();
//		qDebug("%s::%s() : %d widgets are assigned ", metaObject()->className(), __FUNCTION__, count);
//		isAllocationEnabled = true;
//		ui->allocationButton->setText("Reset Allocation");
//	}
//	else {
//		rMonitor->resetProcessorAllocation();
//		isAllocationEnabled = false;
//		ui->allocationButton->setText("Enable Allocation");
//	}


//	foreach(RailawareWidget *rw, rMonitor->getWidgetList()) {
//		rw->perfMon()->reset(); // reset perf data
//	}
//}

//void ResourceMonitorWidget::on_scheduleButton_clicked()
//{
//	if (schedcontrol) {
//		if (schedcontrol->isRunning()) {
//			// stop scheduling
//			schedcontrol->killScheduler();
//			ui->scheduleButton->setText("Run Scheduling");
//			isScheduleEnabled = false;
//		}
//		else {

//			// run it again
//			schedcontrol->launchScheduler();
//			ui->scheduleButton->setText("Stop Scheduling");
//			isScheduleEnabled = true;
//		}
//	}
//}

















/**
CanvasPicker::CanvasPicker(SchedulingPlot *parent) :
	QObject(parent)
{
	QwtPlotCanvas *canvas = parent->canvas();
	canvas->installEventFilter(this);

#if QT_VERSION >= 0x040000
	canvas->setFocusPolicy(Qt::StrongFocus);
#ifndef QT_NO_CURSOR
	canvas->setCursor(Qt::PointingHandCursor);
#endif
#else
	canvas->setFocusPolicy(QWidget::StrongFocus);
#ifndef QT_NO_CURSOR
	canvas->setCursor(Qt::pointingHandCursor);
#endif
#endif
	canvas->setFocusIndicator(QwtPlotCanvas::ItemFocusIndicator);
	canvas->setFocus();
}

bool CanvasPicker::eventFilter(QObject *object, QEvent *e) {
	if ( object != (QObject *)plot()->canvas() )
		return false;

	switch(e->type()) {
	case QEvent::MouseButtonPress:
	{
		// create new marker if there is no
		// otherwise move marker
		plot()->setFiducialMarkerPos(((QMouseEvent *)e)->pos());
		return true;
	}
	default:
		break;
	}
	return QObject::eventFilter(object, e);
}

SchedulingPlot::SchedulingPlot(ResourceMonitor *r, AbstractScheduler *s, QWidget *parent) :
	QwtPlot(parent),
	rm(r),
	scheduler(s),
	curve(new QwtPlotCurve())
{
	setAutoReplot(false);

	setAxisTitle(QwtPlot::xBottom, "Widgets Priority");
	setAxisScale(QwtPlot::xBottom, 0.0, 1.0);
	setAxisTitle(QwtPlot::yLeft, "Quality Scale");
	setAxisScale(QwtPlot::yLeft, 0.0, 1.0);


	fiducialMarker.setLabel(QwtText("fiducial"));
	fiducialMarker.setLabelAlignment(Qt::AlignBottom | Qt::AlignHCenter);
	fiducialMarker.setValue(0.0, 1.0);
	QwtSymbol markerSymbol;
	markerSymbol.setStyle(QwtSymbol::Diamond);
	markerSymbol.setSize(15);
	fiducialMarker.setSymbol(markerSymbol);
	fiducialMarker.attach(this);


	curve->setPen(QPen(Qt::blue));
	curve->setStyle(QwtPlotCurve::Sticks);
	QwtSymbol symbol;
	symbol.setStyle(QwtSymbol::HLine);
	curve->setSymbol(symbol);



	curve->attach(this);
	replot();
}

void SchedulingPlot::setFiducialMarkerPos(const QPoint &pos) {
	double x = invTransform(QwtPlot::xBottom, pos.x());
	double y = invTransform(QwtPlot::yLeft, pos.y());
	fiducialMarker.setValue(x, y);

	DividerWidgetScheduler *sas = qobject_cast<DividerWidgetScheduler *>(scheduler);
	if (sas) {
		sas->setPAnchor(x);
		sas->setQAnchor(y);
	}
}

void SchedulingPlot::updateData(qint64 ctse) {
	if (!rm) return;
	QList<RailawareWidget *> wList = rm->getWidgetList();

	int size = wList.size();
//	double *x = (double *)malloc(sizeof(double) * size); // priority
//	double *y = (double *)malloc(sizeof(double) * size); // quality scale

	double x[size];
	double y[size];

	qreal lowP = INT_MAX;
	qreal highP = -1;

	for (int i=0; i<size; ++i) {
		RailawareWidget *rw = wList.at(i);
		if (!rw) continue;
		x[i] = rw->priority(ctse);
//		y[i] = rw->perfMon()->currObservedQuality();
		y[i] = rw->observedQuality();

		if ( x[i] < lowP ) lowP = x[i];
		if ( x[i] > highP ) highP = x[i];
	}

	setAxisScale(QwtPlot::xBottom, lowP, highP);

	curve->setData(x, y, size);
	replot();

//	free(x);
//	free(y);
}
**/
