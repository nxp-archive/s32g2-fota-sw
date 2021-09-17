# -*- coding: utf-8 -*-
"""
  Copyright 2021 NXP

  SPDX-License-Identifier:  MIT
"""

if request.global_settings.web2py_version < "2.14.1":
    raise HTTP(500, "Requires web2py 2.13.3 or newer")


from gluon.contrib.appconfig import AppConfig

myconf = AppConfig(reload=True)

if not request.env.web2py_runtime_gae:

    db = DAL(myconf.get('db.uri'),
             pool_size=myconf.get('db.pool_size'),
             migrate_enabled=myconf.get('db.migrate'),
             check_reserved=['all'])#,
             #migrate=True,
             #fake_migrate_all=True)
else:

    db = DAL('google:datastore+ndb')

    session.connect(request, response, db=db)


response.generic_patterns = ['*'] if request.is_local else []

response.formstyle = myconf.get('forms.formstyle') 
response.form_label_separator = myconf.get('forms.separator') or ''



from gluon.tools import Auth, Service, PluginManager


auth = Auth(db, host_names=myconf.get('host.names'), secure=True)
service = Service()
plugins = PluginManager()


auth.define_tables(username=True, signature=True)


mail = auth.settings.mailer
mail.settings.server = 'logging' if request.is_local else myconf.get('smtp.server')
mail.settings.sender = myconf.get('smtp.sender')
mail.settings.login = myconf.get('smtp.login')
mail.settings.tls = myconf.get('smtp.tls') or False
mail.settings.ssl = myconf.get('smtp.ssl') or False


auth.settings.registration_requires_verification = False
auth.settings.registration_requires_approval = False
auth.settings.reset_password_requires_verification = True


db.define_table('ecu_db',
                db.Field.Virtual('firmware', lambda row: str(row.ecu_db.ecu_type)+':'+str(row.ecu_db.update_version)),
                db.Field('supplier_name', 'string', length=25, default=auth.user.username if auth.user else None, readable=False, writable=False),
                db.Field('ecu_type', 'string',fields = ['name'], requires=IS_IN_SET(['GATEWAY', 'BCM', 'PDC'])),
                db.Field('ecuIdentifier', 'string', length=25,  requires=IS_NOT_EMPTY(),writable=False, default='none', readable=False),
                db.Field('hardwareVersion', 'string', length=25,  requires=IS_NOT_EMPTY(),writable=False, default='none', readable=False),
                db.Field('installMethod', 'string',  requires=IS_IN_SET(['abUpdate', 'inPlace']),writable=True, default='abUpdate', readable=False),
                db.Field('imageFormat', 'string', requires=IS_IN_SET(['binary', 'srecord']),writable=True, default='binary', readable=False),
                db.Field('isCompressed', 'string', requires=IS_IN_SET(['noCompress', 'gzip']),writable=True, default='noCompress', readable=False),
                db.Field('dependency', 'integer', length=25,  requires=IS_NOT_EMPTY(),writable=False, default= 1, readable=False),
                db.Field('ecu_version', 'string', length=25,  requires=IS_NOT_EMPTY(),writable=False, default='', readable=False),
                db.Field('metadata', 'string', required=True, requires=IS_NOT_EMPTY(), readable=False, writable=False),
                db.Field('update_version', 'string', length=25,  requires=IS_NOT_EMPTY()),
                db.Field('update_image', 'upload', uploadfolder=request.folder+'/static/uploads' ,required=True, requires=IS_NOT_EMPTY()),
                db.Field('update_filename', 'string', requires=IS_NOT_EMPTY(), default=0, readable=True, writable=False),
                db.Field('isrelease', 'integer', requires=IS_NOT_EMPTY(), default=0, readable=False, writable=False),
                fake_migrate=False,
                migrate=False)

db.define_table('vehicle_db',
                db.Field('vin', 'string', length=25, unique=True),
                db.Field('note', 'string', length=25, requires=IS_NOT_EMPTY(), readable=False),
                db.Field('oem', 'string', length=25, default=auth.user.username if auth.user else None, readable=False, writable=False),
                db.Field('ecu_list', 'list:reference ecu_db', required=IS_IN_DB(db,db.ecu_db._id,db.ecu_db.update_image, multiple = True), readable=False),
            
                #db.Field('supplier_version', 'string', length=25,required=True, writable=False, default='', readable=True),
                db.Field('Image_Repo', 'string', length=25,required=True, writable=False, default='', readable=False),
                #db.Field('director_version', 'string', length=25,required=True, writable=False, default='', readable=True),# requires=IS_NOT_EMPTY()),
                db.Field('Director_Repo', 'string', length=25,required=True, writable=False, default='', readable=False),# requires=IS_NOT_EMPTY()),
                #db.Field.Method('virtual1', lambda row: row.vehicle_db.ecu_list),# requires=IS_NOT_EMPTY()),
                #db.Field.Virtual('displays_ecu_id', lambda row: row.vehicle_db.ecu_list),# requires=IS_NOT_EMPTY()),
                db.Field('vehicle_version', 'string', length=25, required=True, readable=False, writable=False, default='', requires=IS_NOT_EMPTY()),
                db.Field('status', 'string', length=25, required=True, writable=False, readable=False, default='Good'),
                db.Field('time_elapsed', 'string', length=25, default='', writable=False,readable=False,),
                db.Field('checkin_date', 'datetime', length=25, required=True, requires=IS_NOT_EMPTY(), readable=False),
                fake_migrate=False,
                migrate=False)


db.define_table('ecu_overview_db',
                db.Field('Vin', 'string', length=25, unique=True),
                db.Field('ECU_TYPE', 'string', length=25, unique=True,readable=True),
                db.Field('Fw_Name', 'string', length=25, unique=True,readable=True),
                db.Field('Image_repo', 'string', length=25,required=True, writable=False, default='', readable=True),
                db.Field('Director_repo', 'string', length=25,required=True, writable=False, default='', readable=True),
                db.Field('Vehicle', 'string', length=25,required=True, writable=False, default='', readable=True),
                fake_migrate=False,
                migrate=False)

db.define_table('update_image_db',
                db.Field('updated_image', 'string', length=25, unique=True),
                db.Field('updated_vin', 'string', length=25, unique=True,readable=True),
                db.Field('updated_ecu', 'string', length=25, unique=True,readable=True),
                db.Field('updated_version', 'string', length=25,required=True, readable=True),
                db.Field('ecu_id_x', 'integer', length=25,required=True, readable=False),
                fake_migrate=False,
                migrate=False)


