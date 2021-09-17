# -*- coding: utf-8 -*-
# this file is released under public domain and you can use without limitations
"""
  Copyright 2021 NXP

  SPDX-License-Identifier:  MIT
"""
from web2py import gluon
from applications.UPTANE.modules.test_external import create_meta
from collections import OrderedDict

from shutil import copyfile

import datetime
import demo
import os
import re
import xmlrpc.client
import time
import base64

VEH_SHOW_STATUS = 'version'
@auth.requires_login()
def index():


    user = auth.user.username
    # if OEM1
    if user == 'oem1':
        return database_contents()
    # if OEM2
    if user == 'oem2':
        return database_contents()
    # if Supplier 1
    if user == 'supplier1':
        return dict(form=update_form(), db_contents=database_contents())
    # if Supplier 2
    if user == 'supplier2':
        return dict(form=update_form(), db_contents=database_contents())
    else:
        return dict(message=T('Hello unknown user..'))



@auth.requires_login()
def hacked():
    return database_contents()

@auth.requires_login()
def hacked_repo():
    return dict(form=update_form(), db_contents=database_contents())

def user():

    return dict(form=auth())

@cache.action()
def download():

    return response.download(request, db)


def determine_available_updates():
    available_update_list = []
    vehicles =  db(db.vehicle_db.oem == auth.user.username).select()
    print('vehicles:\t {0}'.format(vehicles))
    # Retrieve a list of vehicles associated w/ OEM
    if vehicles:
        for v in vehicles:
            #print('\nv:\t{0}'.format(v))
            # Iterate through ECUs for each vehicle to determine if a newer one exists
            for e in v.ecu_list:
                try:
                    #print('\ne:\t{0}'.format(e))
                    # Retrieve the ecu based off the id
                    ecu = db(db.ecu_db.id == e).select().first()
                    #print('\necu:\t{0}'.format(ecu))
                    # Retrieve the type from the ecu object
                    if ecu:
                        ecu_type = ecu.ecu_type
                        #print('\necu_type:\t{0}'.format(ecu_type))
                        # Query the database for updates for ecu_type and select the last one (i.e., most recent update)
                        ecu_type_updates = db(db.ecu_db.ecu_type == ecu_type).select().last()
                        # If the last update for this ecu_type id is > ecu.id then there's a newer update available
                        # so append the vehicle id to the available update list and break
                        if ecu_type_updates.id > e:
                            available_update_list.append(v.id)
                            break
                        else:
                            continue
                except Exception as e:
                    print('Unable to determine the available updates due to this error: {0}'.format(e))
    #print(available_update_list)
    return available_update_list

def call():
    """
    exposes services. for example:
    http://..../[app]/default/call/jsonrpc
    decorate with @services.jsonrpc the functions to expose
    supports xml, json, xmlrpc, jsonrpc, amfrpc, rss, csv
    """
    return service()

@auth.requires_login()
def all_records():
    print('ALL RECORDS BABY!!!')
    grid = SQLFORM.grid(db.ecu_db.supplier_name==auth.user.username,user_signature=False, csv=False)
    return locals()



@auth.requires_login()
def update_form():
    ''' This is the form the Supplier will see at the top of the screen that allows them to upload a new firmware image
    :return: the form that will be utilized by the user
    '''

    imagerepo = xmlrpc.client.ServerProxy('http://' + str(demo.IMAGE_REPO_SERVICE_HOST) +
                                              ':' + str(demo.IMAGE_REPO_SERVICE_PORT))

    #record = db.ecu_db(request.args(1))
    form=SQLFORM(db.ecu_db)#, record)
    if form.validate():
        # Adding the update to the Image (Supplier) Repo
        cwd = os.getcwd()
        update_image = form.vars.update_image
        update_image_filename = form.vars.update_image_filename
        print(update_image_filename)
    
        print(update_image)
        # After getting the file image name, convert the name of the file from hex to ascii
        # and use this value to populate the imagerepo db with
        filename = return_filename(update_image)
        version = form.vars.update_version.split('.',1)
        ac_update_version = (int(version[0]) << 8) | int(version[1])
        image_custom =  {'releaseCounter':0,
                        'hardwareIdentifier':'test',
                        'ecuIdentifier':'test',
                        'hardwareVersion': 1.0,
                        'installMethod':'test',
                        'imageFormat':'test',
                        'isCompressed':'test',
                        'dependency':1}
        image_custom['hardwareIdentifier'] = form.vars.ecu_type
        image_custom['ecuIdentifier'] = form.vars.ecu_type
        #image_custom['hardwareVersion'] = form.vars.hardwareVersion
        image_custom['installMethod'] = form.vars.installMethod
        image_custom['imageFormat'] = form.vars.imageFormat
        image_custom['isCompressed'] = form.vars.isCompressed
        image_custom['dependency'] = ac_update_version
        print(image_custom)

        # Add uploaded images to imagerepo repo + write to repo
        imagerepo.add_target_to_image_repo(cwd+'/applications/UPTANE/static/uploads/'+update_image,filename,image_custom)
   
        imagerepo.write_image_repo()

        # Metadata was initially intended to be showed, so this place holder function call was created
        meta = create_meta(form.vars.ecu_type + '_' + form.vars.update_version)
 
        # Add or Update this instantiation of the ECU in the ECU db
        
        id_added = db.ecu_db.update_or_insert((db.ecu_db.supplier_name == auth.user.username) &
                                        (db.ecu_db.ecu_type == form.vars.ecu_type) &
                                        (db.ecu_db.update_version == form.vars.update_version),
                                        ecu_type=form.vars.ecu_type,
                                        update_version=form.vars.update_version,
                                        supplier_name=auth.user.username,
                                        metadata=meta,
                                        ecuIdentifier=form.vars.ecu_type,
                                        hardwareVersion=form.vars.ecu_type,
                                        installMethod=form.vars.installMethod,
                                        imageFormat=form.vars.imageFormat,
                                        isCompressed=form.vars.isCompressed,
                                        dependency=form.vars.dependency,
                                        ecu_version=form.vars.ecu_version,
                                        update_image=form.vars.update_image,
                                        update_filename=return_filename(form.vars.update_image) )
        response.flash = 'form accepted'
    elif form.process().errors:
        response.flash = 'form has errors'
    else:
        response.flash = 'please fill out the form'
    return form

@auth.requires_login()
def add_ecu_validation(form):
    meta = create_meta('3')
    id_added = db.ecu_db.update_or_insert((db.ecu_db.supplier_name == auth.user.username) &
                                        (db.ecu_db.ecu_type == form.vars.ecu_type) &
                                        (db.ecu_db.update_version == form.vars.update_version),
                                        ecu_type=form.vars.ecu_type,
                                        update_version=form.vars.update_version,
                                        supplier_name=auth.user.username,
                                        metadata=meta,
                                        update_image=form.vars.update_image)


@auth.requires_login()
def vehicle_ecu_list():
    vehicle_id = request.args(0)
    ecu_type_list = ["BCM","PDC","GATEWAY"]
    vehicle_vin = db(db.vehicle_db.id==vehicle_id).select().first().vin
    ecu_id_list =  db(db.vehicle_db.id==vehicle_id).select().first().ecu_list
    update_ecu_overview_Image_repo(vehicle_vin)
    vin_ecus_content = SQLFORM.grid(db(db.ecu_overview_db.Vin == vehicle_vin), user_signature=False,searchable=False, csv=False,
                                       create=False, editable=False, details=False,)

    
    
    return dict(ecu_type_list = ecu_type_list,
                vehicle_vin = vehicle_vin,
                ecu_id_list = ecu_id_list,
                ecu_list = vin_ecus_content,
                )
                #ecu_list = SQLFORM.grid(db.ecu_db.id =='164')



def create_vehicle(form):
    print('\n\nCREATE_VEHICLE()')
    try:
        director = xmlrpc.client.ServerProxy('http://' + str(demo.DIRECTOR_SERVER_HOST) +
                                            ':' + str(demo.DIRECTOR_SERVER_PORT))
        print('\n\nAFTER CREATING DIRECTOR@ ADDR: {0}:{1}'.format(str(demo.DIRECTOR_SERVER_HOST), str(demo.DIRECTOR_SERVER_PORT)))

        # Add a new vehicle to the director repo (which includes writing to the repo)
        director.add_new_vehicle(form.vars.vin)
        #director.write_director_repo(form.vars.vin)
        print('\n\nform.vars.vin: {0}'.format(str(form.vars.vin)))
        #pri_ecu_key = demo.import_public_key('primary')
        #sec_ecu_key = demo.import_public_key('secondary')
        pri_ecu_key = ''
        sec_ecu_key = ''
        ecu_pub_key = ''
        cwd = os.getcwd()

        for e_id in form.vars.ecu_list:
            ecu = db(db.ecu_db.id==e_id).select().first()
            print("e_id:"+e_id)

            db(db.ecu_db.id==e_id).update(isrelease = 1) 
            filename = return_filename(ecu.update_image)
            filepath = cwd+ str('/applications/UPTANE/uploads/') + filename
            
            # Determine if ECU is primary or secondary
            is_primary = True if ecu.ecu_type == 'GATEWAY' else False
            # Register the ecu w/ the vehicle
            ecu_pub_key = pri_ecu_key if is_primary else sec_ecu_key

            # only register ecus ONCE - correct?
            director.register_ecu_serial(str(ecu.ecu_type) + form.vars.vin , ecu_pub_key, form.vars.vin, is_primary)
            director.add_target_to_director(filepath, filename, form.vars.vin, str(ecu.ecu_type)+ form.vars.vin)
            if str(ecu.ecu_type) == "BCM":
                flash_driver_filepath = cwd + str('/applications/UPTANE/uploads/BCM_Flash_driver.bin')
                director.add_target_to_director(flash_driver_filepath, 'BCM_Flash_driver.bin', form.vars.vin,str(ecu.ecu_type)+ form.vars.vin)
            elif str(ecu.ecu_type) == "GATEWAY":
                flash_driver_filepath = cwd + str('/applications/UPTANE/uploads/GATEWAY_Flash_driver.bin')
                director.add_target_to_director(flash_driver_filepath, 'GATEWAY_Flash_driver.bin', form.vars.vin, str(ecu.ecu_type) + form.vars.vin)
            # add into vehicle list

      
            db.ecu_overview_db.insert(
                            ECU_TYPE = ecu.ecu_type,
                            Fw_Name=return_filename(db(db.ecu_db.ecu_type == ecu.ecu_type).select().last().update_image),
                            Image_repo = 1,
                            Director_repo = 1, 
                            Vin = form.vars.vin
                        )  
        # Necessary?
        director.write_director_repo(form.vars.vin)  

    except Exception as e:
        print('Unable to create a new vehicle due to the following issue: {0}'.format(e))


@auth.requires_login()
def get_supplier_versions(list_of_vehicles):
    try:
        if VEH_SHOW_STATUS == 'version':
            gw = db(db.ecu_db.ecu_type=='GATEWAY').select().last()
            pdc = db(db.ecu_db.ecu_type=='PDC').select().last()
            bcm = db(db.ecu_db.ecu_type=='BCM').select().last()
            supplier_version = str(pdc.ecu_type) + " : " + str(pdc.update_version) + '\n' + \
                            str(gw.ecu_type) + " : " + str(gw.update_version) + '\n' + \
                            str(bcm.ecu_type) + " : " + str(bcm.update_version)

            for vehicle in list_of_vehicles:
                # Assume all vehicles have TCU, BCU, and INFO
                vehicle.update_record(Image_Repo=supplier_version)
        elif VEH_SHOW_STATUS == 'image':
            gw = db(db.ecu_db.ecu_type=='GATEWAY').select().last()
            pdc = db(db.ecu_db.ecu_type=='PDC').select().last()
            bcm = db(db.ecu_db.ecu_type=='BCM').select().last()
            supplier_image = str(pdc.ecu_type) + ":" + str(return_filename(pdc.update_image)) + '\n' + \
                            str(gw.ecu_type) + ":" + str(return_filename(gw.update_image)) + '\n' + \
                            str(bcm.ecu_type) + ":" + str(return_filename(bcm.update_image))

            for vehicle in list_of_vehicles:
                # Assume all vehicles have TCU, BCU, and INFO
                vehicle.update_record(Image_Repo=supplier_image)
    except Exception as e:
        print('Unable to get the supplier versions due to the following error: {0}'.format(e))

@auth.requires_login()
def get_director_versions(list_of_vehicles):

    try:
        if VEH_SHOW_STATUS == 'version':
            for vehicle in list_of_vehicles:
                director_version = ''
                version_dict = {}
                
                for ecu in vehicle.ecu_list:
                    ecu_type       = db(db.ecu_db.id==ecu).select().first().ecu_type
                    update_version = db(db.ecu_db.id==ecu).select().first().update_version
                    version_dict[ecu_type] = update_version
                    director_version += ' ' + str(ecu_type) + ' : ' + str(update_version)             
                db.vehicle_db(db.vehicle_db.id == vehicle).update_record(Director_Repo = director_version)

        elif VEH_SHOW_STATUS == 'image':
            '''
                for vehicle in list_of_vehicles:
                    director_image = ''
                    image_dict = {}
                    
                    for ecu in vehicle.ecu_list:
                        ecu_type = db(db.ecu_db.id==ecu).select().last().ecu_type
                        filename = return_filename(db(db.ecu_db.id==ecu).select().last().update_image)
                        image_dict[ecu_type] = filename
                        director_image += ' ' + str(ecu_type) + ' : ' + str(filename)+ '\n' 

                    db.vehicle_db(db.vehicle_db.id == vehicle).update_record(Director_Repo = director_image)
            '''
            gw = db(db.ecu_db.ecu_type=='GATEWAY').select().last()
            pdc = db(db.ecu_db.ecu_type=='PDC').select().last()
            bcm = db(db.ecu_db.ecu_type=='BCM').select().last()
            supplier_image = str(pdc.ecu_type) + ":" + str(return_filename(pdc.update_image)) + '\n' + \
                            str(gw.ecu_type) + ":" + str(return_filename(gw.update_image)) + '\n' + \
                            str(bcm.ecu_type) + ":" + str(return_filename(bcm.update_image))

            for vehicle in list_of_vehicles:
                # Assume all vehicles have TCU, BCU, and INFO
                vehicle.update_record(Director_Repo=supplier_image)
     
    except Exception as e:
        print('Unable to get the director versions due to the following error: {0}'.format(e))

@auth.requires_login()
def get_vehicle_versions(list_of_vehicles):
    director = xmlrpc.client.ServerProxy('http://' + str(demo.DIRECTOR_SERVER_HOST) +
                                            ':' + str(demo.DIRECTOR_SERVER_PORT))

    # Iterate through the list of vehicles in order to return the vehicle reported versions
    for vehicle in list_of_vehicles:
        #print('vehicle: {0} : vin#: {1}'.format(vehicle, vehicle.vin))
        try:
            # Currently we're iterating through the 3 ECU types we limit the Supplier in uploading in order to retrieve
            #  their ECU manifests.  These are used to append to the ecu_serial_string.
            ecu_list = ['PDC', 'GATEWAY', 'BCM']
            ecu_serial_string = ''
            # If the default file is returned from the ECU manifests (secondary_firmware.txt), then is_default stays true
            #  and the checkin time for the vehicle will not be updated.  However, if the ECU manifests have a different
            #  file (i.e., the vehicle has 'updated'), then is_default gets set to False and the checkin time shall be
            #  updated.
            is_default = True
            update_checkin_time = False
            # Iterate through the list of ECU's (which is currently hardcoded) and gather their ecu_manifest
            #   From each ecu_manifest, parse the file name of their current image and add it to the ecu_serial_string
            for ecu in ecu_list:
                ecu_manifest = director.get_last_ecu_manifest(str(ecu))
                print(ecu_manifest)
                try:
                    file_path = ecu_manifest['signed']['installed_image']['filepath']
                    #TODO add true vlaue
                    version = ecu_manifest['signed']['installed_image']['version']
                    major_v = (version>>8)&0xFF
                    min_v = (version)&0xFF
                    up_version = str(major_v)+'.'+str(min_v)
                    
                    #version = 2
                    db(db.ecu_overview_db.ECU_TYPE == ecu).update(Vehicle = up_version)
                    if file_path == '/secondary_firmware.txt':
                        ecu_serial_string += '{0} : {1}  '.format(str(ecu), 'N/A/test')
                    else:
                        is_default = False
                        update_checkin_time = True
                        ecu_serial_string += '{0} : {1}  '.format(str(ecu), file_path[-7:-4])
                except Exception:
                    # This is to catch the error received from an ECU manifest that returns an error
                    ecu_serial_string += '{0} : {1}  '.format(str(ecu), 'N/A/test') if is_default else '{0} : {1}  '.format(str(ecu), '1.0')

            vehicle.update_record(vehicle_version=ecu_serial_string)
            cur_time = datetime.datetime.now()

            vehicle.update_record(checkin_date=cur_time) if update_checkin_time else print('Checkin date will not be updated for vehicle: {0}'.format(vehicle.vin))
        except Exception as e:
            vehicle.update_record(vehicle_version='None')
            print('did not work with this error: {0}'.format(e))



@auth.requires_login()
def get_status(list_of_vehicles):
    director = xmlrpc.client.ServerProxy('http://' + str(demo.DIRECTOR_SERVER_HOST) +
                                            ':' + str(demo.DIRECTOR_SERVER_PORT))

    # Iterate through the list of vehicles in order to return the ECU reported attacks detected
    for vehicle in list_of_vehicles:
        try:
            # Get the vehicle's manifest
            #print('\nvehicle[get_status]: {0}'.format(vehicle))
            vv = director.get_last_vehicle_manifest(vehicle.vin)
            attack_status = ''
            if 'signed' in vv:
                try:
                    # Retrieve all of the reported ecu_version manifests keys
                    ecu_serials = list(vv['signed']['ecu_version_manifests'].keys())
                    if ecu_serials:
                        for ecu in ecu_serials:
                            # Retrieve the ECU's last manifest to parse the attacks_detected
                            ecu_mani = director.get_last_ecu_manifest(ecu)
                            if ecu_mani:
                                attack_status = ecu_mani['signed']['attacks_detected']
                except Exception:
                    print('Unable to get the ecu_version_manifests from the vehicle manifest.')

            # If the attack_status is not the default of '' then we'll return the reported attack_status
            #   otherwise, we'll return a value of 'Good'
            attack_status = attack_status if attack_status else 'Good'
            vehicle.update_record(status=attack_status)

        except Exception as e:
            vehicle.update_record(status='Unknown')
            print('get_status failed with this error: {0}'.format(e))

@auth.requires_login()
def get_time_elapsed(list_of_vehicles):
    for vehicle in list_of_vehicles:
        cur_time = datetime.datetime.now()
        checkin_date = vehicle.checkin_date
        elapsed_time = cur_time - checkin_date

        # Convert the elapsed_time to days (d), hours (h), minutes (m), and seconds (s)
        d = divmod(elapsed_time.total_seconds(), 86400)
        h = divmod(d[1], 3600)
        m = divmod(h[1], 60)
        s = m[1]

        elapsed_time_string = '{0} days'.format(int(d[0]))
        #elapsed_time_string = '{0} days, {1} hours, {2} minutes'.format(int(d[0]),int(h[0]),int(m[0]))
        vehicle.update_record(time_elapsed=elapsed_time_string)


@auth.requires_login()
def database_contents():
    # return the database contents based off the current_user
    current_user = auth.user.username
    #print(request.args(0))
    # If it's a supplier, pull up data based off their username
    if current_user == 'supplier1' or current_user == 'supplier2':
        if db.ecu_db.id != '':
            # The following sets the ID#'s to non-readable and pulls all ECU's created by the current user
            db.ecu_db.id.readable=False
            db_contents = SQLFORM.grid(db.ecu_db.supplier_name==auth.user.username, searchable=False, csv=False,
                                       create=False, editable=False, details=False,)
                                       #onvalidation=add_ecu_validation)
        else:
            db_contents = T('Hello, you have no ecu/vehicles....')
        return db_contents

    # Else it's an OEM; so display database applicable to them
    else:
        list_of_vehicles = db(db.vehicle_db.oem==auth.user.username).select()
        get_supplier_versions(list_of_vehicles)
        get_director_versions(list_of_vehicles)
        get_vehicle_versions(list_of_vehicles)
        get_status(list_of_vehicles) 
        get_time_elapsed(list_of_vehicles)
        db.vehicle_db.id.readable = False
        db_contents = SQLFORM.grid(db.vehicle_db.oem==auth.user.username, create=True,oncreate=create_vehicle,csv=False,
                                    searchable=False, details=False, editable=False, 
                                    links = [lambda row: A('ECU_list', _href=URL("vehicle_ecu_list",args=[row.id]))],
                                    maxtextlengths={'vehicle_db.Image_Repo':50,
                                                   'vehicle_db.vehicle_version':50,
                                                   'vehicle_db.Director_Repo':50})
        '''
        # If we are adding a new vehicle to the database
        if 'new' in request.args:
            # Regex used to find all ecu 'options' within the db_contents div tag
            #   String we are parsing db_contents for, thus reasoning for specific regex: '<div>[ecu_id]</div>'
            reg_val = re.findall("\>\d{1,3}\<", str(db_contents))
            # If there is a match, pull value from ecu_db
            if reg_val:
                for reg in reg_val:
                    ecu_id= reg[1:-1]
                    ecu = db(db.ecu_db.id==ecu_id).select().first()
                    ecu_name = ecu.ecu_type
                    ecu_ver  = ecu.update_version
                    ecu_str  = '>' + ecu_name + ' - ' + ecu_ver + '<'
                    # Find old 'option' string and replace with newly retrieved ECU Type + Version
                    db_contents = gluon.html.XML(str(db_contents).replace(reg, ecu_str))
        '''
        changed_ecu_list = request.vars['changed_ecu_list']
        edited_vehicle = request.vars['vehicle_id']
        error_message = request.vars['error_message']

        available_updates = []
        available_updates = determine_available_updates()

        #add the ecu table 
        db.ecu_db.id.readable=False
        ecu_contents = SQLFORM.grid(db.ecu_db.isrelease==0,selectable=lambda ecu_id:ecu_update_list(ecu_id),selectable_submit_button='publish',
                                       searchable=False, csv=False,create=False, editable=False, details=False,                               
                                       )

        if changed_ecu_list == None:
            return dict(db_contents=db_contents,changed_ecu_list=changed_ecu_list, ecu_contents=ecu_contents,available_updates=available_updates, error_message=error_message)
        else:
            return dict(db_contents=db_contents,  ecu_contents=ecu_contents, changed_ecu_list=changed_ecu_list,edited_vehicle=edited_vehicle,
                        available_updates=available_updates, error_message=error_message)

@auth.requires_login()
def ecu_update_list(ecu_id_list):

    
    # qury the vehicle 
    director = xmlrpc.client.ServerProxy('http://' + str(demo.DIRECTOR_SERVER_HOST) +
                                            ':' + str(demo.DIRECTOR_SERVER_PORT))
    #get the vehicle in the DB
    for vehicle in db(db.vehicle_db).select():
        for ecu_id in ecu_id_list:
            ecu_update_fw = db(db.ecu_db.id == ecu_id).select().first()
            print("ecu_update_list\n")
            vin = vehicle.vin
            ecu_serial = ecu_update_fw.ecu_type
            # Add the bundle to the vehicle
            cwd = os.getcwd()
            # Retrieve the filename
            filename = return_filename(ecu_update_fw.update_image)
            # <~> Why are we using test_uploads paths? We can't assume the
            # image files exist in the test_uploads folder.
            filepath = cwd + str('/applications/UPTANE/static/uploads/'+ecu_update_fw.update_image)

            if(ecu_update_fw.ecu_type == "BCM"):
                print("ecu_update_fw.ecu_type == BCM")                
                flash_driver_filepath = cwd + str('/applications/UPTANE/uploads/BCM_Flash_driver.bin')
                director.add_target_to_director(flash_driver_filepath, 'BCM_Flash_driver.bin', vin, ecu_serial)
                director.add_target_to_director(filepath, filename, vin,  ecu_serial)

            elif(ecu_update_fw.ecu_type == "GATEWAY"):
                print("ecu_update_fw.ecu_type == GATEWAY")  
                flash_driver_filepath = cwd + str('/applications/UPTANE/uploads/GATEWAY_Flash_driver.bin')
                director.add_target_to_director(flash_driver_filepath, 'GATEWAY_Flash_driver.bin', vin, ecu_serial)
                director.add_target_to_director(filepath, filename, vin, ecu_serial)

            elif(ecu_update_fw.ecu_type == "PDC"):
                print("ecu_update_fw.ecu_type == PDC")  
                director.add_target_to_director(filepath, filename, vin, ecu_serial)
            
            db(db.ecu_db.id==ecu_id).update(isrelease = 1)
        
            new_ecu_list =  vehicle.ecu_list.append(ecu_id)
            print("new_ecu_list"+str(new_ecu_list))
            vehicle.update(ecu_list = new_ecu_list)

    director.write_director_repo(vin)

@auth.requires_login()
def selected_vehicle(vehicle_id):
    print('Here I am inside of the selectable...')
    # Ensure that a single vehicle is selected before continuing
    error_message = None
    if len(vehicle_id) == 1:
        vehicle = db(db.vehicle_db.id==vehicle_id[0]).select().first()
        vehicle_ecu_list = []
        ecu_id_list = []
        ecu_type_list  = []
        for ecu in vehicle.ecu_list:
            selected_ecu = db(db.ecu_db.id==ecu).select().first()
            # Add all ecu_id's to the ecu_id_list
            vehicle_ecu_list.append(selected_ecu)
            ecu_id_list.append(selected_ecu.id)
            # Only add ecu_types if they are not currently in the ecu_type_list
            if selected_ecu.ecu_type not in ecu_type_list:ecu_type_list.append(selected_ecu.ecu_type)
            #name = selected_ecu.ecu_type
        redirect(URL('ecu_list', vars=dict(vehicle_ecu_list=vehicle_ecu_list, ecu_type_list=ecu_type_list, ecu_id_list=ecu_id_list, vehicle_id=vehicle_id)))
    else:
        print('Please select a single vehicle first.')
        error_message = 'Please select a single vehicle before proceeding.'
        redirect(URL('index', vars=dict(error_message=error_message)))



@auth.requires_login()
def ecu_list():
    print("-------------ecu_list-----------")
    vehicle_ecu_list =  request.vars['vehicle_ecu_list']
    ecu_id_list =  request.vars['ecu_id_list']
    ecu_type_list = request.vars['ecu_type_list']
    vehicle_id = request.vars['vehicle_id']
    vehicle_vin = db(db.vehicle_db.id==vehicle_id).select().first().vin
    # Now have to build the query that will effect what is shown on the screen
    num_ecus = 0
    query = ''
    for ecu in iter(ecu_type_list):
        #print ecu
        num_ecus += 1
        #query+="({0}=='{1}')".format(db.ecu_db.ecu_type,ecu)
        query+="(db.ecu_db.ecu_type=='" + ecu + "')"
        if num_ecus < len(ecu_type_list):
            query+=" | "

    print('query: {0}'.format(query))
    # This line changes our custom query (created above) from a str to a type Query
    # This enables us to send the query as the first argument for SQLFORM.grid()
    mod_query = db(eval(query))
    print('mod query: {0}'.format(mod_query))
    db.ecu_db.id.readable=False
    return dict(ecu_type_list=ecu_type_list, ecu_id_list=ecu_id_list,
                ecu_list=SQLFORM.grid(mod_query, selectable=lambda ecus: selected_ecus(ecus), csv=False,
                                      orderby=[db.ecu_db.ecu_type, ~db.ecu_db.update_version],
                                      searchable=False, editable=False, deletable=False, create=False,details=False,
                                      selectable_submit_button='Create Bundle',
                                      onupdate=create_bundle_update()),vehicle_vin=vehicle_vin)#, selected=ecu_id_list))


@auth.requires_login()
def create_bundle_update():
    print('\n\nUP inside the update')


@auth.requires_login()
def selected_ecus(selected_ecus):
    print('inside selected ecus')
    ecu_id_list = request.vars['ecu_id_list']
    changed_ecu_list = []
    vehicle_id = request.vars['vehicle_id']

    is_primary = False
    for ecu in selected_ecus:
        if str(ecu) not in ecu_id_list:
            changed_ecu_list.append(ecu)
        else:
            print(str(ecu) + ' is in the list!')

    if changed_ecu_list:
        db.vehicle_db(db.vehicle_db.id == vehicle_id).update_record(ecu_list=selected_ecus)
        #if len(changed_ecu_list) == 1: # <~> Why use this condition?
        print('changed_ecu_list contains ' + repr(len(changed_ecu_list)))

        director = xmlrpc.client.ServerProxy('http://' + str(demo.DIRECTOR_SERVER_HOST) +
                                                ':' + str(demo.DIRECTOR_SERVER_PORT))
        # <~> I'd expect vin to be the same within this call, since this
        # dialog is for an individual vehicle. No?
        vin = db(db.vehicle_db.id==vehicle_id).select().first().vin
        director.clear_vehicle_targets(vin)
        for i in range(len(changed_ecu_list)):


            cur_ecu = db(db.ecu_db.id==changed_ecu_list[i]).select().first()
            #print('\ncur_ecu: {0}'.format(cur_ecu))

            # Do a check to see if it's the Primary (potentially add it after appending to
            #   changed_ecu_list w/ boolean is_primary)
            is_primary = True if cur_ecu.ecu_type == 'GATEWAY' else False

            # Add the bundle to the vehicle
            cwd = os.getcwd()

            # Retrieve the filename
            filename = return_filename(cur_ecu.update_image)
            # <~> Why are we using test_uploads paths? We can't assume the
            # image files exist in the test_uploads folder.
            filepath = cwd + str('/applications/UPTANE/test_uploads/'+filename)


            vehicle_id = request.vars['vehicle_id']
            ecu_serial = cur_ecu.ecu_type+str(vin)

         
            director.add_target_to_director(filepath, filename, vin, ecu_serial)
            director.write_director_repo(vin)

            pri_ecu_key =''
            sec_ecu_key = ''
            ecu_pub_key = ''

    redirect(URL('index', vars=dict(ecu_id_list=ecu_id_list, selected_ecu_list=selected_ecus, changed_ecu_list=changed_ecu_list, vehicle_id=vehicle_id)))


def is_base64_code(s):
    _base64_code = ['A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
                    'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',
                    'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '+',
                    '/', '=' ]

    for i in s:
        if(i in _base64_code):
            return True
  
    return False

@auth.requires_login()
def return_filename(update_image):
    print('update_image: {0}'.format(update_image))

    # After getting the file image name, convert the name of the file from hex to ascii
    # and use this value to populate the supplier db with
    fname_after_split=str(update_image).split('.')[-2]
    #fname_after_split = "R0FURVdBWV9BUFBfTTdfMC5iaw4="
    if is_base64_code(fname_after_split) is True:
        filename = base64.b64decode(fname_after_split)
        filename = str(filename,'utf-8')
        print('filename base64: {0}'.format(filename))
    else:    
        filename = bytes.fromhex(fname_after_split).decode('utf-8')
    print('filename: {0}'.format(filename))
    return str(filename)



# This function is intended to be used with a Director/OEM resetting their system
@auth.requires_login()
def reset_system():
    ''' This method is invoked when the Reset System button is clicked on the OEM screen.
    The intention of this function is to remove only the latest added ECU update and vehicle.
    :return: Updated ecu_db and vehicle_db
    '''


    try:
        #vehicles = db(db.vehicle_db.id != None ).select()
        #ecus     = db(db.ecu_db.id != None ).select()
        #ecus_or  = db(db.ecu_overview.id != None).select()

        # If there are more than one ecu's then continue (don't want to delete the last remaining ecu)
        #if not db(db.ecu_db).isempty():
            # Get the last ecu from the pydal.Objects.Rows -- could iterate through the rows if necessary
            #ecu = ecus.last()

            # Uncomment this line below if you want to delete the ecu
            #db(db.ecu_db).delete()
            #pass

        # If there are more than one vehicles then continue (don't want to delete the last remaining vehicle)
        #if  db(db.vehicle_db).isempty():
        # Setup the director to call the XMLRPC call
        # If the vehicle vin is needed can get it from: vehicle.vin (assuming it's a single vehicle and not the list of vehicles)
        #db(db.vehicle_db.id >1).delete()


        db(db.ecu_overview_db.id >0 ).delete()
        db(db.ecu_db.id >0 ).delete()
        db(db.vehicle_db.id >0 ).delete()
        '''
        director = xmlrpc.client.ServerProxy('http://' + str(demo.DIRECTOR_SERVER_HOST) +
                                    ':' + str(demo.DIRECTOR_SERVER_PORT))
        vin='S32G-RDB2'
        
        director.clear_vehicle_targets(vin)
        director.write_director_repo()
        '''
        # Get the last vehicle from the pydal.Objects.Rows -- could iterate through the rows if necessary
        #vehicle = vehicles.last()

    except Exception as e:
        db_contents = ('Unable to reset the system due to this error: {0}'.format(e))
    

    db_contents = T('reset the web client system successful, please reset the uptane server')
    return db_contents




@auth.requires_login()
def vehicles_INFO():
    list_of_vehicles = db(db.vehicle_db.oem==auth.user.username).select()
   
    get_supplier_versions(list_of_vehicles)
    get_director_versions(list_of_vehicles)
    get_vehicle_versions(list_of_vehicles)
    get_status(list_of_vehicles) 
    get_time_elapsed(list_of_vehicles)
    db.vehicle_db.id.readable=False
    db_contents = SQLFORM.grid(db.vehicle_db, create=True,csv=False,user_signature=False,
                        searchable=False, details=False, editable=False, oncreate=create_vehicle,
                        links = [lambda row: A('ECU_list', _href = URL("vehicle_ecu_list",args=[row.id]))],

                        maxtextlengths={'vehicle_db.Image_Repo':500,
                                        'vehicle_db.vehicle_version':500,
                                        'vehicle_db.Director_Repo':500})


    return dict(db_contents = db_contents,)

@auth.requires_login()
def vehicle_ecu_clear():
    pass


@auth.requires_login()
def vin_single_upgrade():
    pass


@auth.requires_login()
def fota_image_publish(id_list):

      # qury the vehicle 
    director = xmlrpc.client.ServerProxy('http://' + str(demo.DIRECTOR_SERVER_HOST) +
                                            ':' + str(demo.DIRECTOR_SERVER_PORT))

    for update_id in  id_list:
        print("TEST update_id : "+str(update_id))
        update_image_hd = db(db.update_image_db.id == update_id).select().first()
        
        cwd = os.getcwd()

        vin = update_image_hd.updated_vin
        ecu_serial = update_image_hd.updated_ecu
        ecu_hd = db(db.ecu_db.id == update_image_hd.ecu_id_x).select().first()
        filename = return_filename(ecu_hd.update_image)



        filepath = cwd + str('/applications/UPTANE/static/uploads/'+filename)
        print("test fota_image_publish filename: "+str(filename))
        print("test fota_image_publish filepath: "+str(filepath))
       
        copyfile(cwd + str('/applications/UPTANE/static/uploads/'+ecu_hd.update_image), (cwd + str('/applications/UPTANE/static/uploads/'+filename)))

        if(ecu_serial == "BCM"):
            print("ecu_update_fw.ecu_type == BCM")                
            flash_driver_filepath = cwd + str('/applications/UPTANE/uploads/BCM_Flash_driver.bin')
            director.add_target_to_director(flash_driver_filepath, 'BCM_Flash_driver.bin', vin, ecu_serial)
            director.add_target_to_director(filepath ,filename, vin,  ecu_serial)

        elif(ecu_serial == "GATEWAY"):
            print("ecu_update_fw.ecu_type == GATEWAY")  
            flash_driver_filepath = cwd + str('/applications/UPTANE/uploads/GATEWAY_Flash_driver.bin')
            director.add_target_to_director(flash_driver_filepath, 'GATEWAY_Flash_driver.bin', vin, ecu_serial)
            director.add_target_to_director(filepath ,filename, vin, ecu_serial)

        elif(ecu_serial == "PDC"):
            print("ecu_update_fw.ecu_type == PDC")  
            director.add_target_to_director(filepath ,filename, vin, ecu_serial)
            
        #if ecu_id_x not in db(db.vehicle_db.vin==).ecu_list:
        #
        vehicle_handle = db(db.vehicle_db.vin == vin).select().first()
        new_ecu_list = vehicle_handle.ecu_list.append(update_image_hd.ecu_id_x)
        
        print("new_ecu_list:"+str(new_ecu_list))
        vehicle_handle.update(ecu_list = new_ecu_list)
        director.write_director_repo(vin)
        update_ecu_overview_Image_repo(vin)
        update_ecu_overview_Director_repo(vin)
    


def update_ecu_overview_Image_repo(vin):
    #vheicle = db(db.vehicle_db.vin==vin).select()
    
    ecu_type_list = ["BCM","PDC","GATEWAY"]
    if(db.ecu_overview_db.Vin == vin):                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
        for ecu_type in ecu_type_list:      
            db(db.ecu_overview_db.ECU_TYPE == ecu_type ).update(Fw_Name = return_filename(db(db.ecu_db.ecu_type == ecu_type).select().last().update_image))
            db(db.ecu_overview_db.ECU_TYPE == ecu_type ).update(Image_repo = db(db.ecu_db.ecu_type == ecu_type).select().last().update_version)
            #db(db.ecu_overview_db.ECU_TYPE == ecu_type ).update(Director_repo = db(db.ecu_db.ecu_type == ecu_type).select().last().update_version)
  
def update_ecu_overview_Director_repo(vin):
    #vheicle = db(db.vehicle_db.vin==vin).select()
    ecu_type_list = ["BCM","PDC","GATEWAY"]
    if(db.ecu_overview_db.Vin == vin):                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
        for ecu_type in ecu_type_list:      
            db(db.ecu_overview_db.ECU_TYPE == ecu_type ).update(Director_repo = db(db.ecu_db.ecu_type == ecu_type).select().last().update_version)

            #db(db.ecu_overview_db.ECU_TYPE == ecu_type).update(Vin = db(db.vehicle_db.id==vehicle_id).select().first().vin)


@auth.requires_login()
def ecu_batch_upgrade():

    db.update_image_db.truncate()
    
    db.ecu_db.id.readable=False
    #go through the vehicles information
    for vehicle in db(db.vehicle_db).select():
        #go through the ECU version of vehicle
        for ecu_ov_handle in db(db.ecu_overview_db.Vin == vehicle.vin).select():
            print("ecu_ov_handle:"+vehicle.vin)
            print("ecu_ov_handle:"+ecu_ov_handle.ECU_TYPE)
            for image_no_release in db(db.ecu_db).select():
                # compare the ECU version and uploaded image 
                if ecu_ov_handle.ECU_TYPE == image_no_release.ecu_type:
                    # if version of uploaded image large than the ECU version
                    if image_no_release.update_version > ecu_ov_handle.Director_repo:                        
                        id_added = db.update_image_db.insert(
                            updated_ecu = image_no_release.ecu_type,
                            updated_version = image_no_release.update_version,
                            updated_vin = ecu_ov_handle.Vin,
                            updated_image = image_no_release.update_filename,
                            ecu_id_x = image_no_release.id
                            ) 
    
    # show the image  
    ecu_upgrade_contents = SQLFORM.grid(db.update_image_db,selectable=lambda id_list:fota_image_publish(id_list),selectable_submit_button='publish',
                                       searchable=False, csv=False,create=False, editable=False, details=False,  )
    #db(db.ecu_db.id==ecu_id).update(isrelease = 1)


    #ecu_upgrade_contents = SQLFORM.grid(db.ecu_db.isrelease==0,selectable=lambda ecu_id_list:ecu_update_list(ecu_id_list),selectable_submit_button='publish',
    #                                   searchable=False, csv=False,create=False, editable=False, details=False,  )
    return dict(ecu_upgrade_contents = ecu_upgrade_contents,)


@auth.requires_login()
def Image_update():
    db.ecu_db.id.readable=False
    db_ecu_content = SQLFORM.grid(db.ecu_db, searchable=False, csv=False,
                                create=False, editable=False, details=False,)
    
    return dict(form=update_form(), db_contents=db_ecu_content,)

@auth.requires_login()
def factory_init_repo():
    # Adding the update to the Image (Supplier) Repo
    imagerepo = xmlrpc.client.ServerProxy('http://' + str(demo.IMAGE_REPO_SERVICE_HOST) +
                                              ':' + str(demo.IMAGE_REPO_SERVICE_PORT))
    # Add uploaded images to imagerepo repo + write to repo
    cwd = os.getcwd()
    factory_ecu_list = []
    update_image = 'GATEWAY_APP_0.bin'
    update_image_db = 'ecu_db.update_image.be469459f8635b0e.474154455741595f4150505f302e62696e.bin'
    VNP_image_custom =  {'releaseCounter':1,'hardwareIdentifier':'GATEWAY','ecuIdentifier':'GATEWAY','hardwareVersion':1,'installMethod':'inPlace','imageFormat':'binary','isCompressed':'noCompress','dependency':1}
    imagerepo.add_target_to_image_repo(cwd+'/applications/UPTANE/uploads/'+update_image,update_image,VNP_image_custom)

    id_added = db.ecu_db.insert(
                            ecu_type='GATEWAY',
                            update_version='0.1',
                            supplier_name='supplier1',
                            metadata='',
                            ecuIdentifier='GATEWAY',
                            hardwareVersion='1',
                            installMethod='inPlace',
                            imageFormat='binary',
                            isCompressed='noCompress',
                            dependency= 1,
                            ecu_version= 1 ,
                            update_image = update_image_db,
                            update_filename = return_filename(update_image_db)) 
    print('id_added: {0}\necu:{1}'.format(id_added, db.ecu_db.id==id_added)) 
    factory_ecu_list.append(id_added)
    factory_db_list=str(id_added)+'|'
    

    update_image = 'GATEWAY_Flash_driver.bin'
    update_image_db = 'ecu_db.update_image.aea185be28f4ad33.474154455741595f466c6173685f6472697665722e62696e.bin'
    imagerepo.add_target_to_image_repo(cwd+'/applications/UPTANE/uploads/'+update_image,update_image,VNP_image_custom)  
    '''  
    id_added = db.ecu_db.insert(
                            ecu_type='GATEWAY',
                            update_version='1.0',
                            supplier_name='supplier1',
                            metadata='',
                            ecuIdentifier='GATEWAY',
                            hardwareVersion='1',
                            installMethod='inPlace',
                            imageFormat='binary',
                            isCompressed='noCompress',
                            dependency= 2,
                            ecu_version= 1 ,
                            update_image= update_image_db)  
    print('id_added: {0}\necu:{1}'.format(id_added, db.ecu_db.id==id_added))
    factory_ecu_list.append(id_added)
    '''
    factory_db_list+=str(id_added)+'|'
    
    update_image = 'PDC_firmware.bin'
    update_image_db = 'ecu_db.update_image.aa5885fb88a6fe96.5044435f6669726d776172652e62696e.bin'
    VDS_image_custom =  {'releaseCounter':1,'hardwareIdentifier':'PDC','ecuIdentifier':'PDC','hardwareVersion':1,'installMethod':'inPlace','imageFormat':'binary','isCompressed':'noCompress','dependency':1}
    imagerepo.add_target_to_image_repo(cwd+'/applications/UPTANE/uploads/'+update_image,update_image,VDS_image_custom)    
    id_added = db.ecu_db.insert(
                        ecu_type='PDC',
                        update_version='0.1',
                        supplier_name='supplier1',
                        metadata='',
                        ecuIdentifier='PDC',
                        hardwareVersion='1',
                        installMethod='inPlace',
                        imageFormat='binary',
                        isCompressed='noCompress',
                        dependency= 1,
                        ecu_version= 1,
                        update_image= update_image_db,
                        update_filename = return_filename(update_image_db))
    factory_ecu_list.append(id_added)
    factory_db_list+=str(id_added)+'|'


    update_image = 'BCM_APP_V1.0.bin'
    update_image_db = 'ecu_db.update_image.942a8b5a57a6578f.42434d5f4150505f56312e302e62696e.bin'
    BCM_image_custom =  {'releaseCounter':1,'hardwareIdentifier':'BCM','ecuIdentifier':'BCM','hardwareVersion':1,'installMethod':'inPlace','imageFormat':'binary','isCompressed':'noCompress','dependency':1}
    imagerepo.add_target_to_image_repo(cwd+'/applications/UPTANE/uploads/'+update_image,update_image,BCM_image_custom)
    id_added = db.ecu_db.insert(
                            ecu_type='BCM',
                            update_version='0.1',
                            supplier_name='supplier1',
                            metadata='',
                            ecuIdentifier='BCM',
                            hardwareVersion='1',
                            installMethod='inPlace',
                            imageFormat='binary',
                            isCompressed='noCompress',
                            dependency= 1,
                            ecu_version= 1 ,
                            update_image= update_image_db,
                            update_filename = return_filename(update_image_db))  
    print('id_added: {0}\necu:{1}'.format(id_added, db.ecu_db.id==id_added))
    factory_ecu_list.append(id_added)
    factory_db_list+=str(id_added)+'|'

    update_image = 'BCM_Flash_driver.bin'
    update_image_db = 'ecu_db.update_image.8179c0a2b30a704b.42434d5f466c6173685f6472697665722e62696e.bin'
    imagerepo.add_target_to_image_repo(cwd+'/applications/UPTANE/uploads/'+update_image,update_image,BCM_image_custom)    
    '''
    id_added = db.ecu_db.insert(
                            ecu_type='BCM',
                            update_version='1.0',
                            supplier_name='supplier1',
                            metadata='',
                            ecuIdentifier='BCM',
                            hardwareVersion='1',
                            installMethod='inPlace',
                            imageFormat='binary',
                            isCompressed='noCompress',
                            dependency= 2,
                            ecu_version= '1.0',
                            update_image= update_image_db)
    print('id_added: {0}\necu:{1}'.format(id_added, db.ecu_db.id==id_added)) 
    factory_ecu_list.append(id_added)
    factory_db_list+=str(id_added)+'|'
    '''
    imagerepo.write_image_repo()
    
    time.sleep(1)
    #add a vehicle to director 

    print('\n\nCREATE_VEHICLE()')
    
    director = xmlrpc.client.ServerProxy('http://' + str(demo.DIRECTOR_SERVER_HOST) +
                                        ':' + str(demo.DIRECTOR_SERVER_PORT))
    print('\n\nAFTER CREATING DIRECTOR@ ADDR: {0}:{1}'.format(str(demo.DIRECTOR_SERVER_HOST), str(demo.DIRECTOR_SERVER_PORT)))

    # Add a new vehicle to the director repo (which includes writing to the repo)
    director.add_new_vehicle('S32G-RDB2')
    pri_ecu_key = ''
    sec_ecu_key = ''
    ecu_pub_key = ''

    director.register_ecu_serial('GATEWAY', ecu_pub_key, 'S32G-RDB2',True)
    director.register_ecu_serial('PDC', ecu_pub_key, 'S32G-RDB2',False)
    director.register_ecu_serial('BCM', ecu_pub_key, 'S32G-RDB2',False)
 
    for e_id in factory_ecu_list:
        ecu = db(db.ecu_db.id==e_id).select().first()

        db(db.ecu_db.id==e_id).update(isrelease = 1)
        # Retrieve the filename to add the target to the director
        # cwd = os.getcwd()
        filename = return_filename(ecu.update_image)
        #('/home/gccs-cq2/workplace/cs-s32g-gw/uptane_server/imagerepo/targets/'+filename)
        filepath = cwd+ str('/applications/UPTANE/uploads/') + filename
        # Determine if ECU is primary or secondary
        #is_primary = True if ecu.ecu_type == 'GATEWAY' else False
 
        director.add_target_to_director(filepath, filename, 'S32G-RDB2',str(ecu.ecu_type))
    

    flash_driver_filepath = cwd + str('/applications/UPTANE/uploads/BCM_Flash_driver.bin')
    director.add_target_to_director(flash_driver_filepath, 'BCM_Flash_driver.bin', 'S32G-RDB2', 'BCM')

    flash_driver_filepath = cwd + str('/applications/UPTANE/uploads/GATEWAY_Flash_driver.bin')
    director.add_target_to_director(flash_driver_filepath, 'GATEWAY_Flash_driver.bin', 'S32G-RDB2', 'GATEWAY')

      
    director.write_director_repo('S32G-RDB2')
    
    cur_time = datetime.datetime.now()
    id_added = db.vehicle_db.insert(
                            ecu_list= factory_ecu_list,
                            vehicle_version='1.0',
                            vin='S32G-RDB2',
                            checkin_date = cur_time)
@auth.requires_login()
def factory_init():

  
    factory_init_repo()
    ecu_list_init()
    
    db_contents = T('factory init successful, please return back...')
    return db_contents

@auth.requires_login()
def ecu_list_init():
    ecu_type_list = ["BCM","PDC","GATEWAY"]                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
    for ecu_type in ecu_type_list:
        id_added = db.ecu_overview_db.insert(
                            ECU_TYPE = ecu_type,
                            Fw_Name=return_filename(db(db.ecu_db.ecu_type == ecu_type).select().last().update_image),
                            Image_repo = 0.1,
                            Director_repo = 0.1, 
                            Vin = 'S32G-RDB2'
                        )  
