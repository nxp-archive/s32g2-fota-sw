

# Setup FOTA server and web client

This project is the FOTA server and web client project for S32G2 OTA project. 

The design of OTA project is base on the [Uptane](https://uptane.github.io/) standard.   Uptane is an open and secure software update system design which protects software delivered over-the-air to the computerized units of automobiles.

The web client is base on the [web2py](http://web2py.com/)  web framework, which is a free open source full-stack framework for rapid development of fast, scalable, secure and portable database-driven web-based applications. It's written in [Python](http://www.python.org)

The FOTA server is the cloud node, and the web client is the tool to maintain and operate the FOTA server. FOTA server include the director server,image server and time server. web client has the ability to upload and publish new version of images for OTA clients.   

## 1 Software and hardware dependence 

1. A PC with ubuntu system 18.04
2. Python 2.7 and 3.6 
3. Git 

## 2 Enable FOTA servers

#### 2.1 Get the source

Both Uptane server and Uptane web application are included in the package.

**uptane_server**: Image server and Director server.

**fota_web_app**: The web client of Uptane server. User can use the client to upload and publish updates for ECUs.

#### 2.2 Install dependencies for Ubuntu 18.04

Uptane server is baded on Python2. Some development libraries are necessary to be installed, i.e. Uptane server's  dependencies.  Use the command below to install:

```shell
sudo apt-get install build-essential libssl-dev libffi-dev python-dev python3-dev mysql-server libmysqlclient-dev python-pip
```

And then, reinstall its dependencies using python-pip:

```shell
cd uptane_server
pip install --force-reinstall -r dev-requirements.txt
```

Check the installation, input the`pip list` and check the DIR of the `uptane`. The DIR of uptane should be the current DIR. Below is an example:

```
pip list | grep uptane
uptane (0.1.0, /home/will/fota_server/uptane_server)
```

If not,  you can try to install it to the user site-packages directory using command below:

```
python setup.py install --user
```



#### 2.3 Start the Uptane FOTA servers

```sh
cd fota_server/uptane_server
chmod +x start_server.sh
./start_server.sh 
```

If the `start_server.sh` failed to run and error log likes below:

```
Traceback (most recent call last):
  File "demo/start_servers.py", line 21, in <module>
    import demo
ImportError: No module named demo
>>> 
```

then you can have a try to re-install the `dev-requirements.txt` like below:

```
cd uptane_server
pip install --force-reinstall -r dev-requirements.txt
```



### 3. Enable the web client

#### 3.1 Clone the web2py framework

```
git clone https://github.com/web2py/web2py.git
cd web2py
git submodule update --init --recursive

```

Create the certification for HTTPS. 

```
  openssl genrsa 1024 > web2py.key
  chmod 400 web2py.key
  openssl req -new -x509 -nodes -sha1 -days 1780 -key web2py.key > web2py.crt
  openssl x509 -noout -fingerprint -text < web2py.crt > web2py.info
```

Run  web2py:

```shell
cd web2py
python3 web2py.py -c web2py.crt -k web2py.key
```

It would request you to set your server admin password. The log seems like below:

```shell
web2py Web Framework
Created by Massimo Di Pierro, Copyright 2007-2021
Version 2.21.1-stable+timestamp.2020.11.28.04.10.44
Database drivers available: sqlite3, imaplib, pymysql, pg8000
WARNING:web2py:GUI not available because Tk library is not installed
choose a password:

please visit:
	https://127.0.0.1:8000/
use "kill -SIGTERM 22494" to shutdown the web2py server
```

#### 3.2  Create application UPTANE in web browser 

Open Firefox web browser and go to the address: https://127.0.0.1:8000 

Click the **admin** button:

![image-20210611150143451](.\1)

Then, create the `UPTANE` applicate of web2py, as below:

![image-20210611150537092](.\2)

The application will be created at the folder **web2py/applications/UPTANE**. Copy all files in the folder **uptane_web_app** , merge and replace files to the folder **web2py/applications/UPTANE**.

Now, you can login to the Fota web application at the address ` https://127.0.0.1:8000/UPTANE`.

####  3.3 Go to the application in your web browser 

In the Firefox, go to the address: https://127.0.0.1:8000/UPTANE and, use the account below to login the system:

Username: oem1

Passwd: asdfg12345



