# coding=utf-8
'''
import logging
from logging import handlers

class Logger(object):
    level_relations = {
        'debug':logging.DEBUG,
        'info':logging.INFO,
        'warning':logging.WARNING,
        'error':logging.ERROR,
        'crit':logging.CRITICAL
    }

    def __init__(self,filename,level='info',when='D',backCount=3,fmt='%(asctime)s | %(pathname)s[line:%(lineno)d] | %(levelname)s| %(message)s'):
        config_path = '/home/songyu/workplace/cs-s32g-gw/uptane_test/'
        with open(config_path + 'config.txt') as fd:
            ctr_case = fd.readline()
            fd.close()
        root_file = '/tftpboot/xuewei/s32v234/newroot/Fota_Test/'+ ctr_case +'/'+filename


        self.logger = logging.getLogger(root_file)
        format_str = logging.Formatter(fmt)
        self.logger.setLevel(self.level_relations.get(level))
        sh = logging.StreamHandler()
        sh.setFormatter(format_str) 
        th = handlers.TimedRotatingFileHandler(filename=root_file,when=when,backupCount=backCount,encoding='utf-8')
        th.setFormatter(format_str)
        self.logger.addHandler(sh) 
        self.logger.addHandler(th)
'''
class Test_Logger():
    def __init__(self):
        pass

    def server_logger(self,ctr_case,server_index,test_result,test_msg):
        config_path = '/home/songyu/workplace/cs-s32g-gw/uptane_test/'
        with open(config_path + 'config.txt') as fd:
            ctr_case = fd.readline()
            fd.close()
        log_file = '/tftpboot/xuewei/s32v234/newroot/Fota_Test/'+ ctr_case +'/'+'server.log'
        with open(log_file,'w+') as log_fd:
            log_fd.writelines(ctr_case + '|' + server_index+ '|' +test_result+ '|' +test_msg + '\n')
            log_fd.close()
