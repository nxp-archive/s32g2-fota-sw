drop database if exists DIRECTOR_DB;

create database DIRECTOR_DB;

use DIRECTOR_DB;

-- ----------------------------
--  Table structure for `Vehicle_table`
-- ----------------------------
create table vehicle_manifests (
    `id` INT UNSIGNED AUTO_INCREMENT,
    `vin` varchar(50) not null,
    `count` INT ,
    `vehicle_manifest` varchar(4016),
    primary key (`id`)
) engine=innodb default charset=utf8;

create table ecu_manifests (
    `id` INT UNSIGNED AUTO_INCREMENT,
    `ecu_serial` varchar(50) not null,
    `count` INT,
    `ecu_manifest` varchar(4016) ,
    primary key (`id`)
) engine=innodb default charset=utf8;

create table vehicle_table (
    `id` INT UNSIGNED AUTO_INCREMENT,
    `vin` varchar(50) not null,
    `ecus` varchar(50),
    primary key (`id`)
) engine=innodb default charset=utf8;

-- ----------------------------
--  Table structure for `ECU_table`
-- ----------------------------
create table ecu_table(
    `id` INT UNSIGNED AUTO_INCREMENT,
    `ecu_serial` varchar(50),
    `vin` varchar(50),
    `isprimary` bool,
    `public_key` varchar(4016) ,
    `releaseCounter` varchar(50) ,
    `hardwareIdentifier` varchar(50) ,
    `ecuIdentifier` varchar(50) ,
    `hardwareVersion` varchar(50),
    `installMethod` varchar(50) ,
    `imageFormat` varchar(50) ,
    `isCompressed` varchar(50) ,
    `dependency` varchar(50),
    `cryptography_method`  varchar(50),
    primary key (`id`)
) engine=innodb default charset=utf8;

create table primary_ecu_table(
    `id` INT UNSIGNED AUTO_INCREMENT,
    `ecu_serial` varchar(50) not null,
    `vin` varchar(50) not null,
    `isprimary` bool ,
    `subecus` varchar(50) ,
    `public_key` varchar(4016),
    `releaseCounter` varchar(50),
    `hardwareIdentifier` varchar(50) ,
    `ecuIdentifier` varchar(50),
    `hardwareVersion` varchar(50),
    `installMethod` varchar(50),
    `imageFormat` varchar(50),
    `isCompressed` varchar(50),
    `dependency` varchar(50),
    `cryptography_method`  varchar(50),
    primary key (`id`)
) engine=innodb default charset=utf8;

create table ecu_key_table (
    `id` INT UNSIGNED AUTO_INCREMENT,
    `ecu_serial` varchar(50),
    `vin` varchar(50),
    `key_id` varchar(4096),
    `key_type` varchar(50),
    `public_key` varchar(4096),
    `keyid_hash_algorithm_1` varchar(50),
    `keyid_hash_algorithm_2` varchar(50),
    primary key (`id`)
) engine=innodb default charset=utf8;
