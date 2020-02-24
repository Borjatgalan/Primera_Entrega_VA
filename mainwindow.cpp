#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    cap = new VideoCapture(0);
    winSelected = false;
    colorImage.create(240,320,CV_8UC3);
    grayImage.create(240,320,CV_8UC1);
    destColorImage.create(240,320,CV_8UC3);
    destGrayImage.create(240,320,CV_8UC1);

    visorS = new ImgViewer(&grayImage, ui->imageFrameS);
    visorD = new ImgViewer(&destGrayImage, ui->imageFrameD);

    connect(&timer,SIGNAL(timeout()),this,SLOT(compute()));
    connect(ui->captureButton,SIGNAL(clicked(bool)),this,SLOT(start_stop_capture(bool)));
    connect(ui->colorButton,SIGNAL(clicked(bool)),this,SLOT(change_color_gray(bool)));
    connect(visorS,SIGNAL(windowSelected(QPointF, int, int)),this,SLOT(selectWindow(QPointF, int, int)));
    connect(visorS,SIGNAL(pressEvent()),this,SLOT(deselectWindow()));
    connect(ui->loadFromFile,SIGNAL(pressed()),this,SLOT(loadFromFile()));
    connect(ui->SaveToFile,SIGNAL(pressed()),this,SLOT(saveToFile()));
    connect(ui->Copy,SIGNAL(pressed()),this,SLOT(copy()));
    connect(ui->resize,SIGNAL(pressed()),this,SLOT(resize()));
    connect(ui->enlarge,SIGNAL(pressed()),this,SLOT(enlarge()));


    timer.start(30);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete cap;
    delete visorS;
    delete visorD;
    colorImage.release();
    grayImage.release();
    destColorImage.release();
    destGrayImage.release();
}
//////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::compute()
{
    //Captura de imagen
    if(ui->captureButton->isChecked() && cap->isOpened())
    {
        *cap >> colorImage;
        cv::resize(colorImage, colorImage, Size(320,240));
        cvtColor(colorImage, grayImage, COLOR_BGR2GRAY);
        cvtColor(colorImage, colorImage, COLOR_BGR2RGB);

    }

    //En este punto se debe incluir el código asociado con el procesamiento de cada captura
    modifyRGB();


    //Actualización de los visores

    if(winSelected)
        visorS->drawSquare(QRect(imageWindow.x, imageWindow.y, imageWindow.width,imageWindow.height), Qt::green );

    visorS->update();
    visorD->update();

}

void MainWindow::start_stop_capture(bool start)
{
    if(start)
        ui->captureButton->setText("Stop capture");
    else
        ui->captureButton->setText("Start capture");
}

void MainWindow::change_color_gray(bool color)
{
    if(color)
    {
        ui->colorButton->setText("Gray image");
        visorS->setImage(&colorImage);
        visorD->setImage(&destColorImage);
    }
    else
    {
        ui->colorButton->setText("Color image");
        visorS->setImage(&grayImage);
        visorD->setImage(&destGrayImage);
    }
}

void MainWindow::selectWindow(QPointF p, int w, int h)
{
    QPointF pEnd;
    if(w>0 && h>0)
    {
        imageWindow.x = p.x()-w/2;
        if(imageWindow.x<0)
            imageWindow.x = 0;
        imageWindow.y = p.y()-h/2;
        if(imageWindow.y<0)
            imageWindow.y = 0;
        pEnd.setX(p.x()+w/2);
        if(pEnd.x()>=320)
            pEnd.setX(319);
        pEnd.setY(p.y()+h/2);
        if(pEnd.y()>=240)
            pEnd.setY(239);
        imageWindow.width = pEnd.x()-imageWindow.x+1;
        imageWindow.height = pEnd.y()-imageWindow.y+1;

        winSelected = true;
    }
}

void MainWindow::deselectWindow()
{
    winSelected = false;
}

void MainWindow::modifyRGB()
{
    std::vector<Mat> dest, dest2;
    std::vector<Mat> channels, channels_dest;

    split(colorImage, channels);
    split(destColorImage, channels_dest);

    Mat zero = Mat::zeros(colorImage.size(), CV_8UC1);
    Mat R,G,B;
    Mat R2,G2,B2;
    if(ui->colorButton->isChecked()){
        if (ui->R->isChecked()) {
            R = channels[0];
            R2 = channels_dest[0];
        }
        else{
            R = zero;
            R2 = zero;
        }
        if (ui->G->isChecked()) {
            G = channels[1];
            G2 = channels_dest[1];
        }
        else{
            G = zero;
            G2 = zero;
        }
        if (ui->B->isChecked()) {
            B = channels[2];
            B2 = channels_dest[2];
        }
        else{
            B = zero;
            B2 = zero;
        }

        dest = {R,G,B};
        dest2 = {R2,G2,B2};
        merge(dest, colorImageAux);
        merge(dest2, destColorImageAux);
        visorS->setImage(&colorImageAux);
        visorD->setImage(&destColorImageAux);

    }
    else
        visorS->setImage(&grayImage);

}

void MainWindow::loadFromFile()
{
    disconnect(&timer,SIGNAL(timeout()),this,SLOT(compute()));

    Mat image;
    QString fileName = QFileDialog::getOpenFileName(this,tr("Open"), "/home",tr("Images (*.jpg *.png "
                                                                                "*.jpeg *.gif);;All Files(*)"));
    image = cv::imread(fileName.toStdString());

    if (fileName.isEmpty())
            return;
        else {
            QFile file(fileName);
            if (!file.open(QIODevice::ReadOnly)) {
                QMessageBox::information(this, tr("Unable to open file"), file.errorString());
                return;
            }
    ui->captureButton->setChecked(false);
    ui->captureButton->setText("Start capture");
    cv::resize(image, colorImage, Size(320, 240));
    cvtColor(colorImage, colorImage, COLOR_BGR2RGB);
    cvtColor(colorImage, grayImage, COLOR_RGB2GRAY);

    connect(&timer,SIGNAL(timeout()),this,SLOT(compute()));

}
}

void MainWindow::saveToFile()
{
    disconnect(&timer,SIGNAL(timeout()),this,SLOT(compute()));
    Mat save_image;
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Image File"),
                                                QString(),
                                                tr("JPG (*.jpg);; PNG (*.png);;"
                                                   "JPEG(*.jpeg);; GIF(*.gif);; All Files (*)"));
    if(ui->colorButton->isChecked())
        cvtColor(destColorImage, save_image, COLOR_RGB2BGR);

    else
        cvtColor(destGrayImage, save_image, COLOR_GRAY2BGR);

    if (fileName.isEmpty())
            return;
        else {
            QFile file(fileName);
            if (!file.open(QIODevice::WriteOnly)) {
                QMessageBox::information(this, tr("Unable to open file"),
                    file.errorString());
                return;
            }
         }
    cv::imwrite(fileName.toStdString(), save_image);

    connect(&timer,SIGNAL(timeout()),this,SLOT(compute()));
}

void MainWindow::copy()
{
    if(winSelected){
        Rect winD;
        Mat zero_color = Mat::zeros(destColorImage.size(), CV_8UC3);
        Mat zero_gray = Mat::zeros(destGrayImage.size(), CV_8UC1);

        winD.height = imageWindow.height;
        winD.width = imageWindow.width;
        winD.x = (320 - winD.width) / 2;
        winD.y = (240 - winD.height) / 2;
        if(ui->colorButton->isChecked()){
            printf("Copia realizada en color...\n");
            zero_color.copyTo(destColorImage);
            Mat color_roi = Mat(colorImage,imageWindow);
            Mat destColorROI = Mat(destColorImage,winD);
            color_roi.copyTo(destColorROI);
        }
        else{
            printf("Copia realizada en escala de grises...\n");
            zero_gray.copyTo(destGrayImage);
            Mat gray_roi = Mat(grayImage,imageWindow);
            Mat destGrayROI = Mat(destGrayImage,winD);
            gray_roi.copyTo(destGrayROI);
        }
    }else
    {
        if(ui->colorButton->isChecked())
            colorImage.copyTo(destColorImage);
        else
            grayImage.copyTo(destGrayImage);
    }
}

//Corregir: cuando se guarda en color, se modifica el visorDestino a BGR
void MainWindow::resize()
{
    if(winSelected){
        Rect winD;
        winD.height = imageWindow.height;
        winD.width = imageWindow.width;
        winD.x = imageWindow.x;
        winD.y = imageWindow.y;
        Mat color_roi = Mat(colorImage,winD);
        Mat gray_roi = Mat(grayImage,winD);

        if(ui->colorButton->isChecked())
        {
            printf("Imagen redimensionada en color...\n");
            cv::resize(color_roi, destColorImage, Size(320,240));
            destColorImage.copyTo(destColorImageAux);
        }else{
            printf("Imagen redimensionada en escala de grises...\n");
            cv::resize(gray_roi, destGrayImage, Size(320,240));
            destGrayImage.copyTo(destGrayImageAux);
        }
    }
}

void MainWindow::enlarge(){
    if(winSelected){
        float fx, fy;
        Rect winD;
        Mat zero_color = Mat::zeros(destColorImage.size(), CV_8UC3);
        Mat zero_gray = Mat::zeros(destGrayImage.size(), CV_8UC1);

        winD.height = imageWindow.height;
        winD.width = imageWindow.width;
        winD.x = imageWindow.x;
        winD.y = imageWindow.y;

        fx = 320/winD.width;
        fy = 240/winD.height;

        Mat color_roi = Mat(colorImage,winD);
        Mat gray_roi = Mat(grayImage,winD);

        if(ui->colorButton->isChecked())
        {
            if(fx > fy){
                printf("Imagen alargada en color a lo largo...\n");
                zero_color.copyTo(destColorImage);
                Mat destColorROI = Mat(destColorImage,winD);
                color_roi.copyTo(destColorROI);
                cv::resize(destColorROI, destColorImage, Size(imageWindow.height,240));
                destColorImage.copyTo(destColorImageAux);
            }
            else{
                printf("Imagen alargada en color a lo ancho...\n");
                zero_color.copyTo(destColorImage);
                Mat destColorROI = Mat(destColorImage,winD);
                color_roi.copyTo(destColorROI);
                cv::resize(color_roi, destColorImage, Size(320,imageWindow.width));
                destColorImage.copyTo(destColorImageAux);
            }
        }
        else{
            if(fx > fy){
                printf("Imagen alargada en blanco y negro a lo largo...\n");
                zero_gray.copyTo((destGrayImage));
                Mat destGrayROI = Mat(destGrayImage, winD);
                gray_roi.copyTo((destGrayROI));
                cv::resize(gray_roi, destGrayImage, Size(imageWindow.height,240));
                destGrayImage.copyTo(destGrayImageAux);
            }
            else{
                printf("Imagen alargada en blanco y negro a lo ancho...\n");
                zero_gray.copyTo((destGrayImage));
                Mat destGrayROI = Mat(destGrayImage, winD);
                gray_roi.copyTo((destGrayROI));
                cv::resize(gray_roi, destGrayImage, Size(320,imageWindow.width));
                destGrayImage.copyTo(destGrayImageAux);
            }
        }

    }
}




