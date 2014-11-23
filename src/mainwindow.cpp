#include <QtGui/QtGui>
#include <boost/shared_ptr.hpp>
#include <pcl/features/normal_3d.h>
#include <pcl/common/transforms.h>
#include <pcl/filters/filter.h>

#include "../include/qtbase.h"
#include "../include/cloudmanager.h"
#include "../include/cloudvisualizer.h"
#include "../include/cloudio.h"
#include "../include/euclideanclusterextractiondialog.h"
#include "../include/euclideanclusterextraction.h"
#include "../include/voxelgriddialog.h"
#include "../include/voxelgrid.h"
#include "../include/movingleastsquaresdialog.h"
#include "../include/movingleastsquares.h"
#include "../include/boundaryestimationdialog.h"
#include "../include/boundaryestimation.h"
#include "../include/outliersremovaldialog.h"
#include "../include/outliersremoval.h"
#include "../include/normalfielddialog.h"
#include "../include/virtualscandialog.h"

#include "../include/pairwiseregistrationdialog.h"
#include "../include/pairwiseregistration.h"
#include "../include/pairwiseregistrationinteractor.h"
#include "../include/registrationdatamanager.h"

#include "../diagram/diagramwindow.h"
#include "../include/globalregistrationdialog.h"
#include "../include/globalregistration.h"

#include "../manual_registration/manual_registration.h"
#include "../include/mathutilities.h"

#include "../include/mainwindow.h"

using namespace registar;

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent)
{
	setupUi(this);

	registerMetaType();

	cloudManager = new CloudManager(this);
	cloudVisualizer = new CloudVisualizer(this);
	pairwiseRegistrationManager = new PairwiseRegistrationManager(this);
	registrationDataManager = new RegistrationDataManager(this);

	cycleRegistrationManager = new CycleRegistrationManager(this);

	euclideanClusterExtractionDialog = 0;
	voxelGridDialog = 0;
	movingLeastSquaresDialog = 0;
	boundaryEstimationDialog = 0;
	outliersRemovalDialog = 0;
	normalFieldDialog = 0;
	virtualScanDialog = 0;
	pairwiseRegistrationDialog = 0;

	diagramWindow = 0;
	globalRegistrationDialog = 0;

	manualRegistration = 0;

	setCentralWidget(cloudVisualizer);

	cloudBrowserDockWidget->setVisible(false);
	pointBrowserDockWidget->setVisible(false);

	connect(aboutQtAction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

	QActionGroup *colorActionGroup = new QActionGroup(this);
	colorActionGroup->addAction(colorNoneAction);
	colorActionGroup->addAction(colorOriginalAction);
	colorActionGroup->addAction(colorCustomAction);
}

MainWindow::~MainWindow(){} 

void MainWindow::registerMetaType()
{
	qRegisterMetaType<CloudDataPtr>("CloudDataPtr");
	qRegisterMetaType<CloudDataConstPtr>("CloudDataConstPtr");

	qRegisterMetaType< QList<bool> >("QList<bool>");
	qRegisterMetaType<Eigen::Matrix4f>("Eigen::Matrix4f");
}

void MainWindow::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);
	switch(event->type())
	{
	case QEvent::LanguageChange: 
		retranslateUi(this);
		break;
	default:
		break;
	}
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	if (okToContinue())
	{
		writeSettings();
		event->accept();
	} 
	else 
	{
		event->ignore();
	}
}

bool MainWindow::okToContinue()
{
	int r = QMessageBox::warning(this, tr("Exit Registar"),
						tr("Do you really want to exit?"),
						QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
	switch(r)
	{
	case QMessageBox::Yes:
		return true;
	case QMessageBox::No:
	case QMessageBox::Cancel:
		return false;
	}
	return true;
}

void MainWindow::readSettings()
{

}

void MainWindow::writeSettings()
{

}

QString MainWindow::strippedName(const QString &fullFileName)
{
	return QFileInfo(fullFileName).fileName();
}

void MainWindow::on_aboutAction_triggered()
{
	QMessageBox::about(this, tr("About Registar"),
	 tr("<h2>Registar 1.1</h2>"
	 	"<p>Copyright &copy; 2014 Software Inc."
	 	"<p>Registar is a small application used for "
	 	"registering and aligning multiview point clouds."));
}

void MainWindow::on_openAction_triggered()
{
	QStringList fileNameList = QFileDialog::getOpenFileNames(this, tr("Open PointCloud"), ".", tr("PointCloud files (*.ply)"));

	QStringList::Iterator it = fileNameList.begin();
	while (it != fileNameList.end())
	{
		if (!(*it).isEmpty()) 
		{
			QString fileName = (*it);
			PolygonMeshPtr polygonMesh(new PolygonMesh);
			CloudDataPtr cloudData(new CloudData);
			Eigen::Matrix4f transformation;
			BoundariesPtr boundaries(new Boundaries);
			QApplication::setOverrideCursor(Qt::WaitCursor);
			if ( CloudIO::importPolygonMesh(fileName, polygonMesh) )
			{
				pcl::fromPCLPointCloud2(polygonMesh->cloud, *cloudData);
			}
			else if ( !CloudIO::importCloudData(fileName, cloudData) ) 
			{
				it++;
				continue;
			}
			CloudIO::importTransformation(fileName, transformation);
			CloudIO::importBoundaries(fileName, boundaries);
			QApplication::restoreOverrideCursor();
			QApplication::beep();
			Cloud* cloud = cloudManager->addCloud(cloudData, polygonMesh->polygons, Cloud::fromIO, fileName, transformation);
			cloud->setBoundaries(boundaries);
			cloudBrowser->addCloud(cloud);
			cloudVisualizer->addCloud(cloud);
			cloudVisualizer->resetCamera(cloud);
		}
		it++;
	} 
}


bool MainWindow::on_saveAction_triggered()
{
	QStringList cloudNameList = cloudBrowser->getSelectedCloudNames();
	
	QStringList::Iterator it = cloudNameList.begin();
	while (it != cloudNameList.end())
	{
		QString cloudName = (*it);
		Cloud *cloud = cloudManager->getCloud(cloudName);
		CloudDataConstPtr cloudData = cloud->getCloudData();
		Eigen::Matrix4f transformation = cloud->getTransformation();
		BoundariesConstPtr boundaries = cloud->getBoundaries();
		QString fileName = cloud->getFileName();
		if (fileName.isEmpty())
		{
			QString newfileName = QFileDialog::getSaveFileName(this, tr("Save %1 - %2 as PointCloud").arg(cloudName).arg(strippedName(fileName)), ".", tr("PointCloud files (*.ply)"));
			if (newfileName.isEmpty())
			{
				qDebug() << cloudName << "cancel saveAs!";
				it++;
				continue;
			}
			fileName = newfileName;
		}
		QApplication::setOverrideCursor(Qt::WaitCursor);
		CloudIO::exportCloudData(fileName, cloudData);		
		CloudIO::exportTransformation(fileName, transformation);
		CloudIO::exportBoundaries(fileName, boundaries);
		QApplication::restoreOverrideCursor();
		QApplication::beep();
		qDebug() << cloudName << " saved!";
		it++;
	}

	return true;
}

bool MainWindow::on_saveAsAction_triggered()
{	
	QStringList cloudNameList = cloudBrowser->getSelectedCloudNames();

	QStringList::Iterator it = cloudNameList.begin();
	while (it != cloudNameList.end())
	{
		QString cloudName = (*it);
		Cloud *cloud = cloudManager->getCloud(cloudName);
		CloudDataConstPtr cloudData = cloud->getCloudData();
		Eigen::Matrix4f transformation = cloud->getTransformation();
		BoundariesConstPtr boundaries = cloud->getBoundaries();
		QString fileName = cloud->getFileName();
		QString newFileName = QFileDialog::getSaveFileName(this, tr("Save %1 - %2 as PointCloud").arg(cloudName).arg(strippedName(fileName)), ".", tr("PointCloud files (*.ply)"));
		if (newFileName.isEmpty())
		{
			qDebug() << cloudName << "cancel saveAs!";
			it++;
			continue;
		}
		QApplication::setOverrideCursor(Qt::WaitCursor);
		CloudIO::exportCloudData(newFileName, cloudData);
		CloudIO::exportTransformation(newFileName, transformation);
		CloudIO::exportBoundaries(newFileName, boundaries);
		QApplication::restoreOverrideCursor();
		QApplication::beep();
		qDebug() << cloudName << " saved as " << newFileName;
		it++;
	}

	return true;
}

void MainWindow::on_cloudBrowser_cloudVisibleStateChanged(QString cloudName, bool isVisible)
{
	if (isVisible)
	{
		cloudVisualizer->addCloud(cloudManager->getCloud(cloudName));
	}
	else
	{
		cloudVisualizer->removeCloud(cloudManager->getCloud(cloudName));
	}
}


void MainWindow::on_euclideanClusterExtractionAction_triggered()
{
	if(!euclideanClusterExtractionDialog)
	{
		euclideanClusterExtractionDialog = new EuclideanClusterExtractionDialog(this);
		connect(euclideanClusterExtractionDialog, SIGNAL(sendParameters(QVariantMap)), 
			this, SLOT(on_euclideanClusterExtractionDialog_sendParameters(QVariantMap)));
	}
	euclideanClusterExtractionDialog->show();
	euclideanClusterExtractionDialog->raise();
	euclideanClusterExtractionDialog->activateWindow();
}

void MainWindow::on_voxelGridAction_triggered()
{
	if (!voxelGridDialog)
	{
		voxelGridDialog = new VoxelGridDialog(this);
		connect(voxelGridDialog, SIGNAL(sendParameters(QVariantMap)), 
			this, SLOT(on_voxelGridDialog_sendParameters(QVariantMap)));
	}
	voxelGridDialog->show();
	voxelGridDialog->raise();
	voxelGridDialog->activateWindow();
}

void MainWindow::on_movingLeastSquaresAction_triggered()
{
	if (!movingLeastSquaresDialog)
	{
		movingLeastSquaresDialog = new MovingLeastSquaresDialog(this);
		connect(movingLeastSquaresDialog, SIGNAL(sendParameters(QVariantMap)), 
			this, SLOT(on_movingLeastSquaresDialog_sendParameters(QVariantMap)));
	}
	movingLeastSquaresDialog->show();
	movingLeastSquaresDialog->raise();
	movingLeastSquaresDialog->activateWindow();
}

void MainWindow::on_boundaryEstimationAction_triggered()
{
	if (!boundaryEstimationDialog)
	{
		boundaryEstimationDialog = new BoundaryEstimationDialog(this);
		connect(boundaryEstimationDialog, SIGNAL(sendParameters(QVariantMap)),
			this, SLOT(on_boundaryEstimationDialog_sendParameters(QVariantMap)));
	}
	boundaryEstimationDialog->show();
	boundaryEstimationDialog->raise();
	boundaryEstimationDialog->activateWindow();
}

void MainWindow::on_outliersRemovalAction_triggered()
{
	if (!outliersRemovalDialog)
	{
		outliersRemovalDialog = new OutliersRemovalDialog(this);
		connect(outliersRemovalDialog, SIGNAL(sendParameters(QVariantMap)),
			this, SLOT(on_outliersRemovalDialog_sendParameters(QVariantMap)));
	}
	outliersRemovalDialog->show();
	outliersRemovalDialog->raise();
	outliersRemovalDialog->activateWindow();
}

void MainWindow::on_normalFieldAction_triggered()
{
	if (!normalFieldDialog)
	{
		normalFieldDialog = new NormalFieldDialog(this);
		connect(normalFieldDialog, SIGNAL(sendParameters(QVariantMap)),
			this, SLOT(on_normalFieldDialog_sendParameters(QVariantMap)));
	}
	normalFieldDialog->show();
	normalFieldDialog->raise();
	normalFieldDialog->activateWindow();
}

void MainWindow::on_virtualScanAction_triggered()
{
	if (!virtualScanDialog)
	{
		virtualScanDialog = new VirtualScanDialog(this);
		connect(virtualScanDialog, SIGNAL(sendParameters(QVariantMap)),
			this, SLOT(on_virtualScanDialog_sendParameters(QVariantMap)));
	}
	virtualScanDialog->show();
	virtualScanDialog->raise();
	virtualScanDialog->activateWindow();
}

void MainWindow::on_pairwiseRegistrationAction_triggered()
{
	if (!pairwiseRegistrationDialog)
	{
		pairwiseRegistrationDialog = new PairwiseRegistrationDialog(this);
		connect(pairwiseRegistrationDialog, SIGNAL(sendParameters(QVariantMap)),
			this, SLOT(on_pairwiseRegistrationDialog_sendParameters(QVariantMap)));
	}

	pairwiseRegistrationDialog->targetComboBox->clear();
	pairwiseRegistrationDialog->sourceComboBox->clear();

	QStringList allCloudNames = cloudManager->getAllCloudNames();

	pairwiseRegistrationDialog->targetComboBox->addItems(allCloudNames);
	pairwiseRegistrationDialog->sourceComboBox->addItems(allCloudNames);

	int tabCurrentIndex = pairwiseRegistrationDialog->tabWidget->currentIndex();
	if (tabCurrentIndex != -1)
	{
		pairwiseRegistrationDialog->on_tabWidget_currentChanged(tabCurrentIndex);
	}

	pairwiseRegistrationDialog->show();
	pairwiseRegistrationDialog->raise();
	pairwiseRegistrationDialog->activateWindow();
}

void MainWindow::on_concatenationAction_triggered()
{
	QStringList cloudNameList = cloudBrowser->getSelectedCloudNames();
	
	QStringList::Iterator it = cloudNameList.begin();
	CloudDataPtr con_cloudData(new CloudData);
	BoundariesPtr con_boundaries(new Boundaries);
	while (it != cloudNameList.end())
	{
		QString cloudName = (*it);
		Cloud *cloud = cloudManager->getCloud(cloudName);
		CloudDataPtr cloudData(new CloudData);
		pcl::transformPointCloudWithNormals(*cloud->getCloudData(), *cloudData, cloud->getTransformation());
		(*con_cloudData) += (*cloudData);
		(*con_boundaries) += (*cloud->getBoundaries());
		qDebug() << cloudName << " added!";
		it++;
	}
	
	Polygons polygons(0);
	Cloud* con_cloud = cloudManager->addCloud(con_cloudData, polygons, Cloud::fromFilter);
	con_cloud->setBoundaries(con_boundaries);
	cloudBrowser->addCloud(con_cloud);
	cloudVisualizer->addCloud(con_cloud);
}

void MainWindow::on_diagramAction_triggered()
{
	if (!diagramWindow)
	{
		diagramWindow = new DiagramWindow(this);
		QStringList allCloudNames = cloudManager->getAllCloudNames();
		for (int i = 0; i < allCloudNames.size(); ++i)
		{
			diagramWindow->addNode(allCloudNames.at(i));	
		}
	}

	diagramWindow->show();
	diagramWindow->raise();
	diagramWindow->activateWindow();	
}

void MainWindow::on_globalRegistrationAction_triggered()
{
	if (!globalRegistrationDialog)
	{
		globalRegistrationDialog = new GlobalRegistrationDialog(this);
		connect(globalRegistrationDialog, SIGNAL(sendParameters(QVariantMap)),
			this, SLOT(on_globalRegistrationDialog_sendParameters(QVariantMap)));
	}

	globalRegistrationDialog->show();
	globalRegistrationDialog->raise();
	globalRegistrationDialog->activateWindow();
}

void MainWindow::on_showCloudBrowserAction_toggled(bool isChecked)
{
	cloudBrowserDockWidget->setVisible(cloudBrowserDockWidget->isHidden());
}

void MainWindow::on_showPointBrowserAction_toggled(bool isChecked)
{
	pointBrowserDockWidget->setVisible(pointBrowserDockWidget->isHidden());
}

void MainWindow::on_colorNoneAction_toggled(bool isChecked)
{
	if(isChecked) qDebug() << "None color";

	QStringList cloudNameList = cloudBrowser->getVisibleCloudNames();
	cloudVisualizer->setColorMode(CloudVisualizer::colorNone);
	for(QStringList::iterator it = cloudNameList.begin(); it != cloudNameList.end(); it++)
	{
		QString cloudName = *it;
		Cloud* cloud = cloudManager->getCloud(cloudName);
		cloudVisualizer->updateCloud(cloud);
	}
}

void MainWindow::on_colorOriginalAction_toggled(bool isChecked)
{
	if(isChecked) qDebug() << "Original color";

	QStringList cloudNameList = cloudBrowser->getVisibleCloudNames();
	cloudVisualizer->setColorMode(CloudVisualizer::colorOriginal);
	for(QStringList::iterator it = cloudNameList.begin(); it != cloudNameList.end(); it++)
	{
		QString cloudName = *it;
		Cloud* cloud = cloudManager->getCloud(cloudName);
		cloudVisualizer->updateCloud(cloud);
	}
}

void MainWindow::on_colorCustomAction_toggled(bool isChecked)
{
	if(isChecked) qDebug() << "Custom color";

	cloudVisualizer->setColorMode(CloudVisualizer::colorCustom);

	QStringList cloudNameList = cloudBrowser->getVisibleCloudNames();
	for(QStringList::iterator it = cloudNameList.begin(); it != cloudNameList.end(); it++)
	{
		QString cloudName = *it;
		Cloud* cloud = cloudManager->getCloud(cloudName);
		cloudVisualizer->updateCloud(cloud);
	}
}

void MainWindow::on_drawNormalAction_toggled(bool isChecked)
{
	if(isChecked) qDebug() << "Draw normal";
	else qDebug() << "Undraw normal";
	
	cloudVisualizer->setDrawNormal(isChecked);
	
	QStringList cloudNameList = cloudBrowser->getVisibleCloudNames();
	for(QStringList::iterator it = cloudNameList.begin(); it != cloudNameList.end(); it++)
	{
		QString cloudName = *it;
		Cloud* cloud = cloudManager->getCloud(cloudName);
		cloudVisualizer->updateCloud(cloud);
	}
}

void MainWindow::on_drawAxisAction_toggled(bool isChecked)
{
	if(isChecked) qDebug() << "Draw axis";
	else qDebug() << "Undraw axis";

	if(isChecked)cloudVisualizer->addAxis();
	else cloudVisualizer->removeAxis();
}

void MainWindow::on_drawOrientationMarkerAction_toggled(bool isChecked)
{
	if(isChecked) qDebug() << "Draw orientation marker";
	else qDebug() << "Undraw orientation marker";

	if(isChecked)cloudVisualizer->addOrientationMarker();
	else cloudVisualizer->removeOrientationMarker();
}

void MainWindow::on_registrationModeAction_toggled(bool isChecked)
{
	if(isChecked) qDebug() << "Registration mode";
	else qDebug() << "Close registration mode";

	cloudVisualizer->setRegistrationMode(isChecked);
	
	QStringList cloudNameList = cloudBrowser->getVisibleCloudNames();
	for(QStringList::iterator it = cloudNameList.begin(); it != cloudNameList.end(); it++)
	{
		QString cloudName = *it;
		Cloud* cloud = cloudManager->getCloud(cloudName);
		cloudVisualizer->updateCloud(cloud);
	}
}


void MainWindow::on_drawBoundaryAction_toggled(bool isChecked)
{
	if(isChecked) qDebug() << "Draw boundary";
	else qDebug() << "Undraw Boundary";

	cloudVisualizer->setDrawBoundary(isChecked);

	QStringList cloudNameList = cloudBrowser->getVisibleCloudNames();
	for(QStringList::iterator it = cloudNameList.begin(); it != cloudNameList.end(); it++)
	{
		QString cloudName = *it;
		Cloud* cloud = cloudManager->getCloud(cloudName);
		cloudVisualizer->updateCloud(cloud);
	}
}

void MainWindow::on_confirmRegistrationAction_triggered()
{
	QStringList cloudNameList = cloudBrowser->getSelectedCloudNames();
	QList<bool> isVisibleList = cloudBrowser->getSelectedCloudIsVisible();

	QStringList::Iterator it_name = cloudNameList.begin();
	QList<bool>::Iterator it_visible = isVisibleList.begin();
	while (it_name != cloudNameList.end())
	{
		QString cloudName = (*it_name);
		Cloud *cloud = cloudManager->getCloud(cloudName);
		cloud->setTransformation(cloud->getRegistrationTransformation());
		bool isVisible = (*it_visible);
		if(isVisible)cloudVisualizer->updateCloud(cloud);
		qDebug() << cloudName << "registration transformation confirmed!";
		it_name++;
	}	
}

void MainWindow::on_forceRigidRegistrationAction_triggered()
{
	QStringList cloudNameList = cloudBrowser->getSelectedCloudNames();
	QList<bool> isVisibleList = cloudBrowser->getSelectedCloudIsVisible();

	QStringList::Iterator it_name = cloudNameList.begin();
	QList<bool>::Iterator it_visible = isVisibleList.begin();
	while (it_name != cloudNameList.end())
	{
		QString cloudName = (*it_name);
		Cloud *cloud = cloudManager->getCloud(cloudName);
		//std::cout << getScaleFromTransformation(cloud->getRegistrationTransformation()) << std::endl;
		Eigen::Matrix4f transformation = cloud->getRegistrationTransformation();
		cloud->setRegistrationTransformation(toRigidTransformation(transformation));
		//std::cout << getScaleFromTransformation(cloud->getRegistrationTransformation()) << std::endl;
		bool isVisible = (*it_visible);
		if(isVisible)cloudVisualizer->updateCloud(cloud);
		qDebug() << cloudName << " is forced to rigid transformation!";
		it_name++;
	}		
}


void MainWindow::on_euclideanClusterExtractionDialog_sendParameters(QVariantMap parameters)
{
	QStringList cloudNameList = cloudBrowser->getSelectedCloudNames();
	QList<bool> isVisibleList = cloudBrowser->getSelectedCloudIsVisible();

	QStringList::Iterator it_name = cloudNameList.begin();
	QList<bool>::Iterator it_visible = isVisibleList.begin();
	while (it_name != cloudNameList.end())
	{
		QString cloudName = (*it_name);
		Cloud *cloud = cloudManager->getCloud(cloudName);
		CloudDataConstPtr cloudData = cloud->getCloudData();
		CloudDataPtr cloudData_inliers, cloudData_outliers;
		
		QApplication::setOverrideCursor(Qt::WaitCursor);
		EuclideanClusterExtraction::filter(cloudData, parameters, cloudData_inliers, cloudData_outliers);
		QApplication::restoreOverrideCursor();
		QApplication::beep();

		if( parameters["overwrite"].value<bool>() )
		{
			cloud->setCloudData(cloudData_inliers);
			cloud->setPolygons(Polygons(0));
			cloudBrowser->updateCloud(cloud);
			bool isVisible = (*it_visible);
			if(isVisible)cloudVisualizer->updateCloud(cloud);
		}
		else
		{
			Polygons polygons(0);
			Cloud* cloudInliers = cloudManager->addCloud(cloudData_inliers, polygons, Cloud::fromFilter);
			cloudBrowser->addCloud(cloudInliers);
			cloudVisualizer->addCloud(cloudInliers);

			Cloud* cloudOutliers= cloudManager->addCloud(cloudData_outliers, polygons, Cloud::fromFilter);
			cloudBrowser->addCloud(cloudOutliers);
			cloudVisualizer->addCloud(cloudOutliers);
		}

		it_name++;
		it_visible++;
	}
}

void MainWindow::on_voxelGridDialog_sendParameters(QVariantMap parameters)
{
	QStringList cloudNameList = cloudBrowser->getSelectedCloudNames();
	QList<bool> isVisibleList = cloudBrowser->getSelectedCloudIsVisible();

	QStringList::Iterator it_name = cloudNameList.begin();
	QList<bool>::Iterator it_visible = isVisibleList.begin();
	while (it_name != cloudNameList.end())
	{
		QString cloudName = (*it_name);
		Cloud *cloud = cloudManager->getCloud(cloudName);
		CloudDataConstPtr cloudData = cloud->getCloudData();

		CloudDataPtr cloudData_filtered;
		QApplication::setOverrideCursor(Qt::WaitCursor);
		VoxelGrid::filter(cloudData, parameters, cloudData_filtered);
		QApplication::restoreOverrideCursor();
		QApplication::beep();

		if( parameters["overwrite"].value<bool>() )
		{
			cloud->setCloudData(cloudData_filtered);
			cloud->setPolygons(Polygons(0));
			cloudBrowser->updateCloud(cloud);
			bool isVisible = (*it_visible);
			if(isVisible)cloudVisualizer->updateCloud(cloud);
		}
		else
		{
			Polygons polygons(0);
			Cloud* cloudFiltered = cloudManager->addCloud(cloudData_filtered, polygons, Cloud::fromFilter);
			cloudBrowser->addCloud(cloudFiltered);
			cloudVisualizer->addCloud(cloudFiltered);
		}	
		
		it_name++;
		it_visible++;
	}
}


void MainWindow::on_movingLeastSquaresDialog_sendParameters(QVariantMap parameters)
{
	QStringList cloudNameList = cloudBrowser->getSelectedCloudNames();
	QList<bool> isVisibleList = cloudBrowser->getSelectedCloudIsVisible();

	QStringList::Iterator it_name = cloudNameList.begin();
	QList<bool>::Iterator it_visible = isVisibleList.begin();
	while (it_name != cloudNameList.end())
	{
		QString cloudName = (*it_name);
		Cloud *cloud = cloudManager->getCloud(cloudName);
		CloudDataConstPtr cloudData = cloud->getCloudData();

		CloudDataPtr cloudData_filtered;
		QApplication::setOverrideCursor(Qt::WaitCursor);
		MovingLeastSquares::filter(cloudData, parameters, cloudData_filtered);
		QApplication::restoreOverrideCursor();
		QApplication::beep();

		if( parameters["overwrite"].value<bool>() )
		{
			cloud->setCloudData(cloudData_filtered);
			cloud->setPolygons(Polygons(0));
			cloudBrowser->updateCloud(cloud);
			bool isVisible = (*it_visible);
			if(isVisible)cloudVisualizer->updateCloud(cloud);
		}
		else
		{
			Polygons polygons(0);
			Cloud* cloudFiltered = cloudManager->addCloud(cloudData_filtered, polygons, Cloud::fromFilter);
			cloudBrowser->addCloud(cloudFiltered);
			cloudVisualizer->addCloud(cloudFiltered);
		}	
		
		it_name++;
		it_visible++;
	}	
}

void MainWindow::on_boundaryEstimationDialog_sendParameters(QVariantMap parameters)
{
	QStringList cloudNameList = cloudBrowser->getSelectedCloudNames();
	QList<bool> isVisibleList = cloudBrowser->getSelectedCloudIsVisible();

	QStringList::Iterator it_name = cloudNameList.begin();
	QList<bool>::Iterator it_visible = isVisibleList.begin();
	while (it_name != cloudNameList.end())
	{
		QString cloudName = (*it_name);
		Cloud *cloud = cloudManager->getCloud(cloudName);
		CloudDataConstPtr cloudData = cloud->getCloudData();
		CloudDataPtr cloudData_inliers, cloudData_outliers;
		BoundariesPtr boundaries;
		
		QApplication::setOverrideCursor(Qt::WaitCursor);
		BoundaryEstimation::filter(cloudData, parameters, boundaries, cloudData_inliers, cloudData_outliers);
		QApplication::restoreOverrideCursor();
		QApplication::beep();

		if( parameters["overwrite"].value<bool>() )
		{
			cloud->setCloudData(cloudData_inliers);
			cloud->setPolygons(Polygons(0));
			cloud->setBoundaries(BoundariesPtr());
			cloudBrowser->updateCloud(cloud);
			bool isVisible = (*it_visible);
			if(isVisible)cloudVisualizer->updateCloud(cloud);
		}
		else
		{
			// Cloud* cloudInliers = cloudManager->addCloud(cloudData_inliers, Cloud::fromFilter);
			// cloudBrowser->addCloud(cloudInliers);
			// cloudVisualizer->addCloud(cloudInliers);

			// Cloud* cloudOutliers= cloudManager->addCloud(cloudData_outliers, Cloud::fromFilter);
			// cloudBrowser->addCloud(cloudOutliers);
			// cloudVisualizer->addCloud(cloudOutliers);

			cloud->setBoundaries(boundaries);
			bool isVisible = (*it_visible);
			if(isVisible)cloudVisualizer->updateCloud(cloud);
		}

		it_name++;
		it_visible++;
	}
}


void MainWindow::on_outliersRemovalDialog_sendParameters(QVariantMap parameters)
{
	QStringList cloudNameList = cloudBrowser->getSelectedCloudNames();
	QList<bool> isVisibleList = cloudBrowser->getSelectedCloudIsVisible();

	QStringList::Iterator it_name = cloudNameList.begin();
	QList<bool>::Iterator it_visible = isVisibleList.begin();
	while (it_name != cloudNameList.end())
	{
		QString cloudName = (*it_name);
		Cloud *cloud = cloudManager->getCloud(cloudName);
		CloudDataConstPtr cloudData = cloud->getCloudData();
		CloudDataPtr cloudData_inliers, cloudData_outliers;
		
		QApplication::setOverrideCursor(Qt::WaitCursor);
		OutliersRemoval::filter(cloudData, parameters, cloudData_inliers, cloudData_outliers);
		QApplication::restoreOverrideCursor();
		QApplication::beep();

		if( parameters["overwrite"].value<bool>() )
		{
			cloud->setCloudData(cloudData_inliers);
			cloud->setPolygons(Polygons(0));
			cloudBrowser->updateCloud(cloud);
			bool isVisible = (*it_visible);
			if(isVisible)cloudVisualizer->updateCloud(cloud);
		}
		else
		{
			Polygons polygons(0);
			Cloud* cloudInliers = cloudManager->addCloud(cloudData_inliers, polygons, Cloud::fromFilter);
			cloudBrowser->addCloud(cloudInliers);
			cloudVisualizer->addCloud(cloudInliers);
			
			Cloud* cloudOutliers= cloudManager->addCloud(cloudData_outliers, polygons, Cloud::fromFilter);
			cloudBrowser->addCloud(cloudOutliers);
			cloudVisualizer->addCloud(cloudOutliers);
		}

		it_name++;
		it_visible++;
	}
}

void MainWindow::on_normalFieldDialog_sendParameters(QVariantMap parameters)
{
	float X = parameters["X"].toFloat();
	float Y = parameters["Y"].toFloat();
	float Z = parameters["Z"].toFloat();

	QStringList cloudNameList = cloudBrowser->getSelectedCloudNames();
	QList<bool> isVisibleList = cloudBrowser->getSelectedCloudIsVisible();

	QStringList::Iterator it_name = cloudNameList.begin();
	QList<bool>::Iterator it_visible = isVisibleList.begin();
	while (it_name != cloudNameList.end())
	{
		QString cloudName = (*it_name);
		Cloud *cloud = cloudManager->getCloud(cloudName);
		CloudDataConstPtr cloudData = cloud->getCloudData();

		CloudDataPtr cloudData_filtered;
		QApplication::setOverrideCursor(Qt::WaitCursor);

		cloudData_filtered.reset(new CloudData);
		pcl::copyPointCloud(*cloudData, *cloudData_filtered);
		for (int i = 0; i < cloudData_filtered->size(); ++i)
		{
			pcl::flipNormalTowardsViewpoint(
				(*cloudData_filtered)[i], X, Y, Z, 
				(*cloudData_filtered)[i].normal_x, (*cloudData_filtered)[i].normal_y, 
				(*cloudData_filtered)[i].normal_z); 
		}

		QApplication::restoreOverrideCursor();
		QApplication::beep();

		if( parameters["overwrite"].value<bool>() )
		{
			cloud->setCloudData(cloudData_filtered);
			cloudBrowser->updateCloud(cloud);
			bool isVisible = (*it_visible);
			if(isVisible)cloudVisualizer->updateCloud(cloud);
		}
		else
		{	Polygons polygons = cloud->getPolygons();
			Cloud* cloudFiltered = cloudManager->addCloud(cloudData_filtered, polygons, Cloud::fromFilter);
			cloudBrowser->addCloud(cloudFiltered);
			cloudVisualizer->addCloud(cloudFiltered);
		}	
		
		it_name++;
		it_visible++;
	}	
}

void MainWindow::on_virtualScanDialog_sendParameters(QVariantMap parameters)
{
	int xres, yres;
	xres = parameters["xres"].toInt();
	yres = parameters["yres"].toInt();
	float view_angle = parameters["view_angle"].toFloat();
	bool use_vertices = parameters["use_vertices"].toBool();

	// QSize size_hint(xres, yres);
	// centralWidget()->setFixedSize(size_hint);
	// size_hint = sizeHint();
	// resize(size_hint);

	pcl::PointCloud<pcl::PointXYZ>::Ptr temp(new pcl::PointCloud<pcl::PointXYZ>);
	cloudVisualizer->renderView(temp);

	temp->is_dense = false;
	temp->sensor_origin_ = Eigen::Vector4f(0, 0, 0, 0);
	temp->sensor_orientation_ = Eigen::Quaternionf(1, 0, 0, 0);

	CloudDataPtr cloudData(new CloudData);
	pcl::copyPointCloud(*temp, *cloudData);

	Polygons polygons(0);
	Cloud* cloud = cloudManager->addCloud(cloudData, polygons, Cloud::fromFilter);
	cloudBrowser->addCloud(cloud);
	cloudVisualizer->addCloud(cloud);

	std::vector<int> nanIndicesVector;
	pcl::removeNaNFromPointCloud( *cloudData, *cloudData, nanIndicesVector );
	std::cout << *cloudData << std::endl;

	// pcl::PLYWriter writer;
	// writer.write( "temp.ply", *cloudData);

}

void MainWindow::on_pairwiseRegistrationDialog_sendParameters(QVariantMap parameters)
{
	//qDebug() << "Pairwise Registration Start!";

	QString cloudName_target = parameters["target"].toString();
	QString cloudName_source = parameters["source"].toString();

	QString prName = PairwiseRegistration::generateName(cloudName_target, cloudName_source);
	PairwiseRegistration *pairwiseRegistration = pairwiseRegistrationManager->getPairwiseRegistration(prName);

	if (parameters["command"] == "ShowResults")
	{
		if (pairwiseRegistration != NULL)
		{
			pairwiseRegistrationDialog->showResults(
				pairwiseRegistration->getTransformation(), 
				pairwiseRegistration->getRMSError(),
				pairwiseRegistration->getSquareErrors().size());
		}
	}

	if(parameters["command"] == "Initialize")
	{
		if(pairwiseRegistration == NULL)
		{
			RegistrationData *registrationData_target = registrationDataManager->getRegistrationData(cloudName_target);
			if ( registrationData_target == NULL)
			{
				Cloud *cloud_target = cloudManager->getCloud(cloudName_target);
				registrationData_target = registrationDataManager->addRegistrationData(cloud_target, cloudName_target);
			}
			RegistrationData *registrationData_source = registrationDataManager->getRegistrationData(cloudName_source);
			if ( registrationData_source == NULL)
			{
				Cloud *cloud_source = cloudManager->getCloud(cloudName_source);
				registrationData_source = registrationDataManager->addRegistrationData(cloud_source, cloudName_source);
			}
			//pairwiseRegistration = pairwiseRegistrationManager->addPairwiseRegistration(registrationData_target, registrationData_source, prName);
			pairwiseRegistration = new PairwiseRegistrationInteractor(registrationData_target, registrationData_source, prName);
			pairwiseRegistrationManager->addPairwiseRegistration(pairwiseRegistration);

			CloudVisualizer * cloudVisualizer = pairwiseRegistrationDialog->addCloudVisualizerTab(pairwiseRegistration->objectName());
			dynamic_cast<PairwiseRegistrationInteractor*>(pairwiseRegistration)->setCloudVisualizer(cloudVisualizer);

			if(cloudVisualizer) cloudVisualizer->addCloud(registrationData_target->cloudData, "target", 0, 0, 255);
			if(cloudVisualizer) cloudVisualizer->addCloud(registrationData_source->cloudData, "source", 255, 0, 0);

			pairwiseRegistrationDialog->showResults(
				pairwiseRegistration->getTransformation(), 
				pairwiseRegistration->getRMSError(),
				pairwiseRegistration->getSquareErrors().size());
			//pairwiseRegistration->cloudVisualizer->repaint();
		}
		else 
		{
			pairwiseRegistration->initialize();
			pairwiseRegistrationDialog->showResults(
				pairwiseRegistration->getTransformation(), 
				pairwiseRegistration->getRMSError(),
				pairwiseRegistration->getSquareErrors().size());
		}	
	}

	if(parameters["command"] == "Pre-Correspondences")
	{
		if (pairwiseRegistration == NULL)
		{
			qDebug() << "Still Uninitialized";
		}
		else 
		{
			parameters["isShowBoundaries"] = true;
			parameters["isShowCorrespondences"] = true;
			QApplication::setOverrideCursor(Qt::WaitCursor);
			pairwiseRegistration->process(parameters);
			QApplication::restoreOverrideCursor();
			QApplication::beep();
			pairwiseRegistrationDialog->showResults(
				pairwiseRegistration->getTransformation(), 
				pairwiseRegistration->getRMSError(), 
				pairwiseRegistration->getSquareErrors().size());
		}
	}

	if(parameters["command"] == "ICP")
	{
		if (pairwiseRegistration == NULL)
		{
			qDebug() << "Still Uninitialized";
		}
		else
		{
			parameters["isShowBoundaries"] = false;
			parameters["isShowCorrespondences"] = false;
			QApplication::setOverrideCursor(Qt::WaitCursor);			
			pairwiseRegistration->process(parameters);
			QApplication::restoreOverrideCursor();
			QApplication::beep();
			pairwiseRegistrationDialog->showResults(
				pairwiseRegistration->getTransformation(), 
				pairwiseRegistration->getRMSError(),
				pairwiseRegistration->getSquareErrors().size());

		}
	}

	if(parameters["command"] == "Export")
	{
		if (pairwiseRegistration == NULL)
		{
			qDebug() << "Still Uninitialized";
		}
		else 
		{
			pairwiseRegistration->process(parameters);
			Cloud *cloud_source= cloudManager->getCloud(cloudName_source);
			cloudVisualizer->updateCloud(cloud_source);
			pairwiseRegistrationDialog->showResults(
				pairwiseRegistration->getTransformation(), 
				pairwiseRegistration->getRMSError(),
				pairwiseRegistration->getSquareErrors().size());
		}
	}

	if(parameters["command"] == "Manual")
	{
		if (pairwiseRegistration == NULL)
		{
			qDebug() << "Still Uninitialized";
		}
		else 
		{
			if (manualRegistration == NULL)
			{
				manualRegistration = new ManualRegistration(this);
				connect(manualRegistration, SIGNAL(sendParameters(QVariantMap)), 
					this, SLOT(on_pairwiseRegistrationDialog_sendParameters(QVariantMap)));
			}
			manualRegistration->setWindowTitle(cloudName_source + "->" + cloudName_target);
			manualRegistration->setSrcCloud(pairwiseRegistration->getSource()->cloudData, cloudName_source);
			manualRegistration->setDstCloud(pairwiseRegistration->getTarget()->cloudData, cloudName_target);
			manualRegistration->clearSrcVis();
			manualRegistration->clearDstVis();
			manualRegistration->showSrcCloud();
			manualRegistration->showDstCloud();
			manualRegistration->show();
			manualRegistration->raise();
			manualRegistration->activateWindow();
		}
	}

	if (parameters["command"] == "ManualTransform")
	{
		if (pairwiseRegistration == NULL)
		{
			qDebug() << "Still Uninitialized";
		}
		else 
		{
			qDebug() << "OK";
			pairwiseRegistration->initializeTransformation(parameters["transformation"].value<Eigen::Matrix4f>());
			pairwiseRegistrationDialog->showResults(
				pairwiseRegistration->getTransformation(), 
				pairwiseRegistration->getRMSError(),
				pairwiseRegistration->getSquareErrors().size());
		}
	}
}

void MainWindow::on_globalRegistrationDialog_sendParameters(QVariantMap parameters)
{
	if (parameters["command"] == "InitializePair")
	{
		//qDebug() << parameters["targets"].toStringList();
		//qDebug() << parameters["sources"].toStringList();

		QStringList targets = parameters["targets"].toStringList();
		QStringList sources = parameters["sources"].toStringList();

		for (int i = 0; i < targets.size(); ++i)
		{
			QString cloudName_target = targets[i];
			QString cloudName_source = sources[i];
			QString prName = PairwiseRegistration::generateName(cloudName_target, cloudName_source);
			PairwiseRegistration *pairwiseRegistration = pairwiseRegistrationManager->getPairwiseRegistration(prName);
			if(pairwiseRegistration == NULL)
			{
				RegistrationData *registrationData_target = registrationDataManager->getRegistrationData(cloudName_target);
				if ( registrationData_target == NULL)
				{
					Cloud *cloud_target = cloudManager->getCloud(cloudName_target);
					registrationData_target = registrationDataManager->addRegistrationData(cloud_target, cloudName_target);
				}
				RegistrationData *registrationData_source = registrationDataManager->getRegistrationData(cloudName_source);
				if ( registrationData_source == NULL)
				{
					Cloud *cloud_source = cloudManager->getCloud(cloudName_source);
					registrationData_source = registrationDataManager->addRegistrationData(cloud_source, cloudName_source);
				}
				//pairwiseRegistration = pairwiseRegistrationManager->addPairwiseRegistration(registrationData_target, registrationData_source, prName);
				pairwiseRegistration = new PairwiseRegistrationInteractor(registrationData_target, registrationData_source, prName);
				pairwiseRegistrationManager->addPairwiseRegistration(pairwiseRegistration);

				CloudVisualizer * cloudVisualizer = pairwiseRegistrationDialog->addCloudVisualizerTab(pairwiseRegistration->objectName());
				dynamic_cast<PairwiseRegistrationInteractor*>(pairwiseRegistration)->setCloudVisualizer(cloudVisualizer);

				if(cloudVisualizer) cloudVisualizer->addCloud(registrationData_target->cloudData, "target", 0, 0, 255);
				if(cloudVisualizer) cloudVisualizer->addCloud(registrationData_source->cloudData, "source", 255, 0, 0);

				pairwiseRegistrationDialog->showResults(
					pairwiseRegistration->getTransformation(), 
					pairwiseRegistration->getRMSError(),
					pairwiseRegistration->getSquareErrors().size());
				//pairwiseRegistration->cloudVisualizer->repaint();
			}
			else
			{
				pairwiseRegistration->initialize();
				pairwiseRegistrationDialog->showResults(
					pairwiseRegistration->getTransformation(), 
					pairwiseRegistration->getRMSError(),
					pairwiseRegistration->getSquareErrors().size());
			}
		}
	}

	if (parameters["command"] == "InitializeCycle")
	{
		QStringList targets = parameters["targets"].toStringList();
		QStringList sources = parameters["sources"].toStringList();
		QList<PairwiseRegistration*> prList;

		// qDebug() << "InitializeCycle";
		// qDebug() << targets;
		// qDebug() << sources;

		for (int i = 0; i < targets.size(); ++i)
		{
			QString cloudName_target = targets[i];
			QString cloudName_source = sources[i];
			QString prName = PairwiseRegistration::generateName(cloudName_target, cloudName_source);
			PairwiseRegistration *pairwiseRegistration = pairwiseRegistrationManager->getPairwiseRegistration(prName);
			if(pairwiseRegistration == NULL)
			{
				RegistrationData *registrationData_target = registrationDataManager->getRegistrationData(cloudName_target);
				if ( registrationData_target == NULL)
				{
					Cloud *cloud_target = cloudManager->getCloud(cloudName_target);
					registrationData_target = registrationDataManager->addRegistrationData(cloud_target, cloudName_target);
				}
				RegistrationData *registrationData_source = registrationDataManager->getRegistrationData(cloudName_source);
				if ( registrationData_source == NULL)
				{
					Cloud *cloud_source = cloudManager->getCloud(cloudName_source);
					registrationData_source = registrationDataManager->addRegistrationData(cloud_source, cloudName_source);
				}
				//pairwiseRegistration = pairwiseRegistrationManager->addPairwiseRegistration(registrationData_target, registrationData_source, prName);
				pairwiseRegistration = new PairwiseRegistrationInteractor(registrationData_target, registrationData_source, prName);
				pairwiseRegistrationManager->addPairwiseRegistration(pairwiseRegistration);
				
				CloudVisualizer * cloudVisualizer = pairwiseRegistrationDialog->addCloudVisualizerTab(pairwiseRegistration->objectName());
				dynamic_cast<PairwiseRegistrationInteractor*>(pairwiseRegistration)->setCloudVisualizer(cloudVisualizer);

				if(cloudVisualizer) cloudVisualizer->addCloud(registrationData_target->cloudData, "target", 0, 0, 255);
				if(cloudVisualizer) cloudVisualizer->addCloud(registrationData_source->cloudData, "source", 255, 0, 0);

				pairwiseRegistrationDialog->showResults(
					pairwiseRegistration->getTransformation(), 
					pairwiseRegistration->getRMSError(),
					pairwiseRegistration->getSquareErrors().size());
				//pairwiseRegistration->cloudVisualizer->repaint();
			}
			else
			{
				pairwiseRegistration->initialize();
				pairwiseRegistrationDialog->showResults(
					pairwiseRegistration->getTransformation(), 
					pairwiseRegistration->getRMSError(),
					pairwiseRegistration->getSquareErrors().size());
			}
			prList.append(pairwiseRegistration);
		}

		cycleRegistrationManager->addCycleRegistration(prList);
	}

	if (parameters["command"] == "UniformRefine")
	{
		//qDebug() << "make circle consistent";

		QStringList targets = parameters["targets"].toStringList();
		QStringList sources = parameters["sources"].toStringList();
		QList<bool> freezeds = parameters["freezeds"].value< QList<bool> >();

		QString cloudNameCycle = "";
		for (int i = 0; i < targets.size()-1; ++i)
		{
			cloudNameCycle.append(targets[i]).append(",");
		}
		cloudNameCycle.append(targets.back());
		//qDebug() << cloudNameCycle;

		CycleRegistration* cycleRegistration = cycleRegistrationManager->getCycleRegistration(cloudNameCycle);
		if (cycleRegistration == NULL)
		{
			qDebug() << "Still Uninitialized";
		}
		else
		{
			cycleRegistration->uniform_refine();
		}
	}

	if (parameters["command"] == "ShowEstimation")
	{
		QStringList targets = parameters["targets"].toStringList();
		QStringList sources = parameters["sources"].toStringList();
		QList<Eigen::Matrix4f> transformationList;
		for (int i = 0; i < targets.size(); ++i)
		{
			QString cloudName_target = targets[i];
			QString cloudName_source = sources[i];
			QString prName = PairwiseRegistration::generateName(cloudName_target, cloudName_source);
			PairwiseRegistration *pairwiseRegistration = pairwiseRegistrationManager->getPairwiseRegistration(prName);
			QString reversePrName = PairwiseRegistration::generateName(cloudName_source, cloudName_target);
			PairwiseRegistration *reversePairwiseRegistration = pairwiseRegistrationManager->getPairwiseRegistration(reversePrName);
			if(pairwiseRegistration == NULL && reversePairwiseRegistration == NULL)
			{
				qDebug() << cloudName_target << "<-" << cloudName_source << " not ready yet!!!";
				return;				
			}
			if(pairwiseRegistration) transformationList.append(pairwiseRegistration->getTransformation());
			else transformationList.append(reversePairwiseRegistration->getTransformation().inverse());
		}
		Eigen::Matrix4f transformation_total = Eigen::Matrix4f::Identity();
		for (int i = 0; i < transformationList.size(); ++i) transformation_total *= transformationList[i];
		
		QString prName = PairwiseRegistration::generateName(targets.back(), sources.back());
		PairwiseRegistration *pairwiseRegistration = pairwiseRegistrationManager->getPairwiseRegistration(prName);
		QString reversePrName = PairwiseRegistration::generateName(sources.back(), targets.back());
		PairwiseRegistration *reversePairwiseRegistration = pairwiseRegistrationManager->getPairwiseRegistration(reversePrName);
		float error1 = 0.0f, error2 = 0.0f;
		int ovlNumber1 = 0, ovlNumber2 = 0;

		if(pairwiseRegistration) 
		{
			//std::cerr << (transformation_total * transformationList.back().inverse()).inverse() << std::endl;
			pairwiseRegistration->estimateRMSErrorByTransformation((transformation_total * transformationList.back().inverse()).inverse(), error1, ovlNumber1);
			pairwiseRegistration->estimateVirtualRMSErrorByTransformation((transformation_total * transformationList.back().inverse()).inverse(), error2, ovlNumber2);
		}
		else
		{
			reversePairwiseRegistration->estimateRMSErrorByTransformation(transformation_total * transformationList.back().inverse(), error1, ovlNumber1);
			reversePairwiseRegistration->estimateVirtualRMSErrorByTransformation(transformation_total * transformationList.back().inverse(), error2, ovlNumber2);
		} 
		globalRegistrationDialog->showEstimation(transformation_total, error1, error2, ovlNumber1, ovlNumber2);

	}

	if(parameters["command"] == "Export")
	{
		QString relations = parameters["relations"].toString();
		QStringList relationList = relations.split("\n");

		for (int i = 0; i < relationList.size(); ++i)
		{
			if (relationList[i] != "")
			{
				QStringList cloudList = relationList[i].split("<-");

				Cloud *cloud_begin= cloudManager->getCloud(cloudList[0]);
				cloud_begin->setRegistrationTransformation(cloud_begin->getTransformation());
				cloudVisualizer->updateCloud(cloud_begin);

				Eigen::Matrix4f transformation_temp = Eigen::Matrix4f::Identity();
				for (int j = 1; j < cloudList.size(); ++j)
				{
					QString cloudName_target = cloudList[j-1];
					QString cloudName_source = cloudList[j];
					qDebug() << cloudName_target << "<-" << cloudName_source;

					QString prName = PairwiseRegistration::generateName(cloudName_target, cloudName_source);
					PairwiseRegistration *pairwiseRegistration = pairwiseRegistrationManager->getPairwiseRegistration(prName);
					QString reversePrName = PairwiseRegistration::generateName(cloudName_source, cloudName_target);
					PairwiseRegistration *reversePairwiseRegistration = pairwiseRegistrationManager->getPairwiseRegistration(reversePrName);

					if(pairwiseRegistration == NULL && reversePairwiseRegistration == NULL)
					{
						qDebug() << cloudName_target << "<-" << cloudName_source << " not ready yet!!!";
						return;				
					}
					if(pairwiseRegistration) transformation_temp *= pairwiseRegistration->getTransformation();
					else transformation_temp *= reversePairwiseRegistration->getTransformation().inverse();

					Cloud *cloud_source = cloudManager->getCloud(cloudName_source);
					cloud_source->setRegistrationTransformation( transformation_temp * cloud_source->getTransformation());
					cloudVisualizer->updateCloud(cloud_source);
				}
			}

		}
	}
}


