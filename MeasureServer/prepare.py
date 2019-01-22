#!/usr/bin/python
import sqlite3

conn = sqlite3.connect("measure.db")
c = conn.cursor()
c.execute("PRAGMA foreign_keys = ON;")
c.execute("""	CREATE TABLE `Input` (
		`Id` char NOT NULL PRIMARY KEY,
		`Name` text NOT NULL,
		`Limit` double NOT NULL
		);""");

c.execute("""	CREATE TABLE `Measurement` (
		`Date` datetime NOT NULL,
		`Input` char NOT NULL REFERENCES `Input`(`Id`),
		`Amps` double NOT NULL,
		CONSTRAINT PK_Measure PRIMARY KEY(`Date`, `Input`)
		);""");
c.execute("INSERT INTO `Input`(`Id`, `Name`, `Limit`) VALUES('0', 'Mosogep', 0.8);");
c.execute("INSERT INTO `Input`(`Id`, `Name`, `Limit`) VALUES('1', 'Szaritogep', 0.8);");
conn.commit();
conn.close()


