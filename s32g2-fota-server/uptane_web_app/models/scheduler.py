#import applications.UPTANE.modules.uptane.demo.demo_oem_repo as do
#import applications.UPTANE.modules.uptane.demo.demo_director as dd
#import applications.UPTANE.modules.uptane.demo.demo_timeserver as ts


#def run_once(f):
#    def wrapper(*args, **kwargs):
#        if not wrapper.has_run:
#            wrapper.has_run = True
#            return f(*args,**kwargs)
#        else:
#            print('\n\nWRAPPER HAS RUN:\t{0}'.format(wrapper.has_run))
#    wrapper.has_run = False
#    return wrapper

#@run_once
#def clean_slate():
#    print('\ndo.clean_slate()')
    #do.clean_slate()
    #dd.clean_slate()
#    ts.listen()
    # Ensure to add 'db.commit()' for any insert/updates to the db
    #db.commit()

#from gluon.scheduler import Scheduler
#scheduler = Scheduler(db)
#print('inside scheduler')
#clean_slate()
#scheduler.queue_task(clean_slate())

