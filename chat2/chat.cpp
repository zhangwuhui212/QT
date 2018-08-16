#include "chat.h"
#include "ui_chat.h"

//chat::chat():ui(new Ui::chat)
//{
//	is_opened = false;
//}


chat::chat(QString pasvusername, QString pasvuserip) : ui(new Ui::chat)
{
    ui->setupUi(this);
    ui->textEdit->setFocusPolicy(Qt::StrongFocus);
    ui->textBrowser->setFocusPolicy(Qt::NoFocus);

    ui->textEdit->setFocus();
    ui->textEdit->installEventFilter(this);

	a = 0;
	is_opened = false;
//	this->is_opened = false;
    xpasvusername = pasvusername;
    xpasvuserip = pasvuserip;

    ui->label->setText(tr("��%1������   �Է�IP:%2").arg(xpasvusername).arg(pasvuserip));

	//UDP����
    xchat = new QUdpSocket(this);
    xport = 45456;
 //   xchat->bind(xport, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
	xchat->bind( QHostAddress::QHostAddress(getIP()), xport );
    connect(xchat, SIGNAL(readyRead()), this, SLOT(processPendingDatagrams()));

	//TCP����
    server = new TcpServer(this);
    connect(server,SIGNAL(sendFileName(QString)),this,SLOT(sentFileName(QString)));

    connect(ui->textEdit,SIGNAL(currentCharFormatChanged(QTextCharFormat)),this,SLOT(currentFormatChanged(const QTextCharFormat)));
}

chat::~chat()
{
    is_opened = false;
	delete ui;
}

bool chat::eventFilter(QObject *target, QEvent *event)
{
    if(target == ui->textEdit)
    {
        if(event->type() == QEvent::KeyPress)//���¼���ĳ��
        {
             QKeyEvent *k = static_cast<QKeyEvent *>(event);
             if(k->key() == Qt::Key_Return)//�س���
             {
                 on_send_clicked();
                 return true;
             }
        }
    }
    return QWidget::eventFilter(target,event);
}

//�����û��뿪
void chat::participantLeft(QString userName,QString localHostName,QString time)
{
    ui->textBrowser->setTextColor(Qt::gray);
    ui->textBrowser->setCurrentFont(QFont("Times New Roman",10));
    ui->textBrowser->append(tr("%1 �� %2 �뿪��").arg(userName).arg(time));
}

QString chat::getUserName()  //��ȡ�û���
{
    QStringList envVariables;
    envVariables << "USERNAME.*" << "USER.*" << "USERDOMAIN.*"
                 << "HOSTNAME.*" << "DOMAINNAME.*";
    QStringList environment = QProcess::systemEnvironment();
    foreach (QString string, envVariables)
    {
        int index = environment.indexOf(QRegExp(string));
        if (index != -1)
        {

            QStringList stringList = environment.at(index).split('=');
            if (stringList.size() == 2)
            {
                return stringList.at(1);
                break;
            }
        }
    }
    return false;
}

QString chat::getIP()  //��ȡip��ַ
{
    QList<QHostAddress> list = QNetworkInterface::allAddresses();
    foreach (QHostAddress address, list)
    {
       if(address.protocol() == QAbstractSocket::IPv4Protocol) //����ʹ��IPv4��ַ
            return address.toString();
    }
       return 0;
}

void chat::hasPendingFile(QString userName,QString serverAddress,  //�����ļ�
                            QString clientAddress,QString fileName)
{
    QString ipAddress = getIP();
    if(ipAddress == clientAddress)
    {
        int btn = QMessageBox::information(this,tr("�����ļ�"),
                                 tr("����%1(%2)���ļ���%3,�Ƿ���գ�")
                                 .arg(userName).arg(serverAddress).arg(fileName),
                                 QMessageBox::Yes,QMessageBox::No);
        if(btn == QMessageBox::Yes)
        {
            QString name = QFileDialog::getSaveFileName(0,tr("�����ļ�"),fileName);
            if(!name.isEmpty())
            {
                TcpClient *client = new TcpClient(this);
                client->setFileName(name);
                client->setHostAddress(QHostAddress(serverAddress));
                client->show();

            }

        }
        else{
            sendMessage(Refuse,serverAddress);
        }
    }
}

void chat::processPendingDatagrams()   //��������UDP
{
    while(xchat->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(xchat->pendingDatagramSize());
        xchat->readDatagram(datagram.data(),datagram.size());
        QDataStream in(&datagram,QIODevice::ReadOnly);
        int messageType;
        in >> messageType;
        QString userName,localHostName,ipAddress,messagestr;
        QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        switch(messageType)
        {
			case Xchat:
			{
//				ui.show();
				break;
			}
            case Message:
                {
					//��2����䶼û���á�why������
					/*this->hide();
					this->close();*/
                    in >>userName >>localHostName >>ipAddress >>messagestr;
                    ui->textBrowser->setTextColor(Qt::blue);
                    ui->textBrowser->setCurrentFont(QFont("Times New Roman",12));
                    ui->textBrowser->append("[ " +localHostName+" ] "+ time);//��������������
                    ui->textBrowser->append(messagestr);
			//		ui->textBrowser->show();
					//this->textBrowser->setTextColor(Qt::blue);
					//this->textBrowser->setCurrentFont(QFont("Times New Roman",12));
					//this->textBrowser->append("[ " +localHostName+" ] "+ time);//��������������
					//this->textBrowser->append(messagestr);
			
			//		a ++;
			//		if( is_opened == false )//������䣬���ն�B����ʾ�˿���
					{
						this->show();////���bug1.�յ�˽����Ϣ�����ʾ
				//		ui->textBrowser->show();
					//	this->show();
				//		ui->textBrowser->show();
					//	ui.show();
					//	if( this->show() )
				//		this->hide();
				//		0 == a;
						is_opened = true;
					}
                    break;
                }
        case FileName:
            {
                in >>userName >>localHostName >> ipAddress;
                QString clientAddress,fileName;
                in >> clientAddress >> fileName;
                hasPendingFile(userName,ipAddress,clientAddress,fileName);
                break;
            }
        case Refuse:
            {
                in >> userName >> localHostName;
                QString serverAddress;
                in >> serverAddress;
                QString ipAddress = getIP();

                if(ipAddress == serverAddress)
                {
                    server->refused();
                }
                break;
            }
        case ParticipantLeft:
            {
                in >>userName >>localHostName;
                participantLeft(userName,localHostName,time);
                QMessageBox::information(0,tr("���ζԻ��ر�"),tr("�Է������˶Ի�"),QMessageBox::Ok);
				a = 1;
				ui->textBrowser->clear();
				//is_opened = true;
			//	this->is_opened = false;
				ui->~chat();
                close();
			//	delete ui;
			//	ui = 0;
                break;
            }
        }
    }
}

void chat::sentFileName(QString fileName)
{
    this->fileName = fileName;
    sendMessage(FileName);
}

QString chat::getMessage()  //���Ҫ���͵���Ϣ
{
    QString msg = ui->textEdit->toHtml();
    qDebug()<<msg;
    ui->textEdit->clear();
    ui->textEdit->setFocus();
    return msg;
}

//ͨ��˽���׽��ַ��͵��Է���˽��ר�ö˿���
void chat::sendMessage(MessageType type , QString serverAddress)  //������Ϣ
{
    QByteArray data;
    QDataStream out(&data,QIODevice::WriteOnly);
    QString localHostName = QHostInfo::localHostName();
    QString address = getIP();
    out << type << getUserName() << localHostName;


    switch(type)
    {
    case ParticipantLeft:
        {
            break;
        }
    case Message :
        {
            if(ui->textEdit->toPlainText() == "")
            {
                QMessageBox::warning(0,tr("����"),tr("�������ݲ���Ϊ��"),QMessageBox::Ok);
                return;
            }
            message = getMessage();
            out << address << message;
            ui->textBrowser->verticalScrollBar()->setValue(ui->textBrowser->verticalScrollBar()->maximum());
            break;
        }
    case FileName:
            {
                QString clientAddress = xpasvuserip;
                out << address << clientAddress << fileName;
                break;
            }
    case Refuse:
            {
                out << serverAddress;
                break;
            }
    }
    xchat->writeDatagram(data,data.length(),QHostAddress::QHostAddress(xpasvuserip), 45456);

}

void chat::currentFormatChanged(const QTextCharFormat &format)
{//���༭���������ʽ�ı�ʱ�������ò���״̬Ҳ��֮�ı�
    ui->fontComboBox->setCurrentFont(format.font());

    if(format.fontPointSize()<9)  //��������С������Ϊ������С������Ϊ9
    {
        ui->fontsizecomboBox->setCurrentIndex(3); //����ʾ12
    }
    else
    {
        ui->fontsizecomboBox->setCurrentIndex(ui->fontsizecomboBox->findText(QString::number(format.fontPointSize())));

    }

    ui->textbold->setChecked(format.font().bold());
    ui->textitalic->setChecked(format.font().italic());
    ui->textUnderline->setChecked(format.font().underline());
    color = format.foreground().color();
}

void chat::on_fontComboBox_currentFontChanged(QFont f)//��������
{
    ui->textEdit->setCurrentFont(f);
    ui->textEdit->setFocus();
}

void chat::on_fontsizecomboBox_currentIndexChanged(QString size)
{
   ui->textEdit->setFontPointSize(size.toDouble());
   ui->textEdit->setFocus();
}

void chat::on_textbold_clicked(bool checked)
{
    if(checked)
        ui->textEdit->setFontWeight(QFont::Bold);
    else
        ui->textEdit->setFontWeight(QFont::Normal);
    ui->textEdit->setFocus();
}

void chat::on_textitalic_clicked(bool checked)
{
    ui->textEdit->setFontItalic(checked);
    ui->textEdit->setFocus();
}

void chat::on_save_clicked()//���������¼
{
    if(ui->textBrowser->document()->isEmpty())
        QMessageBox::warning(0,tr("����"),tr("�����¼Ϊ�գ��޷����棡"),QMessageBox::Ok);
    else
    {
       //����ļ���
       QString fileName = QFileDialog::getSaveFileName(this,tr("���������¼"),tr("�����¼"),tr("�ı�(*.txt);;All File(*.*)"));
       if(!fileName.isEmpty())
           saveFile(fileName);
    }
}

void chat::on_clear_clicked()//��������¼
{
    ui->textBrowser->clear();
}

bool chat::saveFile(const QString &fileName)//�����ļ�
{
    QFile file(fileName);
    if(!file.open(QFile::WriteOnly | QFile::Text))

    {
        QMessageBox::warning(this,tr("�����ļ�"),
        tr("�޷������ļ� %1:\n %2").arg(fileName)
        .arg(file.errorString()));
        return false;
    }
    QTextStream out(&file);
    out << ui->textBrowser->toPlainText();

    return true;
}

void chat::on_textUnderline_clicked(bool checked)
{
    ui->textEdit->setFontUnderline(checked);
    ui->textEdit->setFocus();
}

void chat::on_textcolor_clicked()
{
    color = QColorDialog::getColor(color,this);
    if(color.isValid())
    {
        ui->textEdit->setTextColor(color);
        ui->textEdit->setFocus();
    }
}



void chat::on_close_clicked()
{
    sendMessage(ParticipantLeft);
	a = 1;
	ui->textBrowser->clear();
	//is_opened = true;
//	this->is_opened = false;
    close();
	ui->~chat();

	//this->close();
	/*delete ui;
	ui = 0;*/
	
}

void chat::on_send_clicked()
{
    sendMessage(Message);
	QString localHostName = QHostInfo::localHostName();
    QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    ui->textBrowser->setTextColor(Qt::blue);
    ui->textBrowser->setCurrentFont(QFont("Times New Roman",12));
    ui->textBrowser->append("[ " +localHostName+" ] "+ time);
    ui->textBrowser->append(message);
//	is_opened = true;
}

void chat::on_sendfile_clicked()
{
    server->show();
    server->initServer();
}
