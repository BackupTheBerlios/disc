-- SQL script for sipauth module

create database aaa;
USE aaa;

--create table
CREATE TABLE acc (
  sip_from varchar(128) NOT NULL default '',
  sip_to varchar(128) NOT NULL default '',
  sip_status varchar(128) NOT NULL default '',
  sip_method varchar(16) NOT NULL default '',
  i_uri varchar(128) NOT NULL default '',
  o_uri varchar(128) NOT NULL default '',
  from_uri varchar(128) NOT NULL default '',
  to_uri varchar(128) NOT NULL default '',
  sip_callid varchar(128) NOT NULL default '',
  username varchar(64) NOT NULL default '',
  fromtag varchar(128) NOT NULL default '',
  totag varchar(128) NOT NULL default '',
  time datetime NOT NULL default '0000-00-00 00:00:00',
  timestamp timestamp(14) NOT NULL
);


--create table
CREATE TABLE authentication (
	username VARCHAR(50) NOT NULL,
	domain VARCHAR(50) NOT NULL,
	passwd_h VARCHAR(32) NOT NULL
);

CREATE TABLE sipgroups (
	user VARCHAR(20) NOT NULL,
	user_group VARCHAR(20) NOT NULL
);


INSERT INTO authentication VALUES ("admin", "localhost", "password");

INSERT INTO sipgroups VALUES("admin", "anonymous");
