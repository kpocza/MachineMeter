#!/usr/bin/python
from flask import Flask, request
import sqlite3
from sqlite3 import Error
from datetime import datetime, timedelta

app = Flask(__name__)

#counter = 0

@app.route('/measure', methods=['GET'])
def measure():
	id = request.args.get('id')
	amps = float(request.args.get('amps'))
	result = addMeasurement(id, amps)
	return "OK" if result else "ERROR";

@app.route('/info', methods=['GET'])
def info():
	return getInfo(False)

@app.route('/fullinfo', methods=['GET'])
def fullInfo():
	return getInfo(True)

@app.route('/webinfo', methods=['GET'])
def webInfo():
	return getWebInfo(False)

@app.route('/fullwebinfo', methods=['GET'])
def fullWebInfo():
	return getWebInfo(True)


def addMeasurement(id, amps):
	try:
		conn = connect()
		c = conn.cursor()

		c.execute("SELECT `Limit` FROM `Input` WHERE `Id`=?", (id))
		limit = c.fetchone()[0]

		if amps > limit:
		#	global counter
		#	counter=counter+1
		#	print(counter)
			now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
			c.execute("PRAGMA foreign_keys=ON;");
			c.execute("INSERT INTO `Measurement`(`Date`,`Input`,`Amps`) VALUES(?,?,?)", (now, id, amps));
			conn.commit()

		return True;
	except Error as e:
		print(e)
		return False;
	except Exception as e:
		print(e);
		return False;
	finally:
		conn.close();

def getInfo(fullInfo):
	try:
		conn = connect()
		inputs = getInputs(conn)
		result="";
		for inp in inputs:
			inputInfo =getInputInfo(inp, conn, fullInfo)
			result+=inputInfo + ";"

		return result + "\n"
	except Error as e:
		print(e)
		return "DBERROR";
	except Exception as e:
		print(e)
		return "ERROR";
	finally:
		conn.close();

def getWebInfo(fullInfo):
	try:
		conn = connect()
		inputs = getInputs(conn)
		result="<!DOCTYPE html><html><head><meta http-equiv=""refresh"" content=""10""></head><body>";
                result+= "<font size=""32"">"
		for inp in inputs:
			inputInfo =getInputInfo(inp, conn, fullInfo)
			result+=inputInfo + "<br />"

		return result + "</font></body></html>"
	except Error as e:
		print(e)
		return "<html><body>DBERROR</body></html>";
	except Exception as e:
		print(e)
		return "<html><body>ERROR</body></html>";
	finally:
		conn.close();

def getInputs(conn):
	inputs=[]
	c = conn.cursor();

	for row in c.execute("SELECT `Id`, `Name`, `Limit`, `GapSize` FROM `Input`"):
		inputs.append(Input(row[0], row[1], row[2], row[3]))

	return inputs

def getInputInfo(input, conn, fullInfo):
	highs = getHighs(input, conn)

	if len(highs) == 0:
		return input.name + " - N/A";

	lastRange = highs[-1]
	hours, remainder = divmod((lastRange.end - lastRange.start).total_seconds(), 3600)
	minutes, seconds = divmod(remainder, 60)

	isRunning=lastRange.end > datetime.now() - timedelta(minutes=input.gapsize)
	minutesSinceLastMeasure = (datetime.now() - lastRange.end).total_seconds() / 60 - input.gapsize

	if fullInfo:
		return (input.name + " - " +
			"Kezd: " + lastRange.start.strftime("%H:%M") +  
			", Veg: " + lastRange.end.strftime("%H:%M") +
			", Ido: " + "%d:%d" % (hours, minutes) + 
			(", Megy" if isRunning else (", All: " + "%d" % (minutesSinceLastMeasure) + "p") ) )
	else:
		return (input.name + " - " + 
			(
				("Megy (" + "%d:%d"%(hours, minutes) + ")") if isRunning else
				(
					"All" if minutesSinceLastMeasure > 30 else
					("All (" + "%d:%d" % (hours, minutes) +", "+ "%d" % (minutesSinceLastMeasure) + "p)")
				)
			)
			)

def getHighs(input, conn):
	highs=[]
	
	lastHigh=datetime(2019,1,1)
	gapSize=timedelta(minutes=input.gapsize)
	
	minQueryDate = datetime.now() + timedelta(hours=-24)
	minDate = datetime.now() + timedelta(hours=-12)

	c = conn.cursor();
	for row in c.execute("SELECT `Date`, `Amps` FROM `Measurement` WHERE `Input`=? AND `Date`>? ORDER BY `Date`", 
			(input.id, minQueryDate)):
		date = datetime.strptime(row[0], "%Y-%m-%d %H:%M:%S")
		amps = row[1]

		if amps > input.limit:
			if lastHigh + gapSize < date or len(highs) == 0:
				highs.append(DateRange(date, date, 1))
			else:
				highs[-1].end = date
				highs[-1].cnt = highs[-1].cnt + 1

			lastHigh = date
	highs=[item for item in highs if item.start > minDate and item.cnt > 1]
	return highs

def connect():
	return sqlite3.connect("measure.db")

class Input:
	def __init__(self, id, name, limit, gapsize):
		self.id = id
		self.name = name
		self.limit = limit
		self.gapsize = gapsize

class Measurement:
	def __init__(self, date, input, amps):
		self.date = date
		self.input = input
		self.amps = amps

class DateRange:
	def __init__(self, start, end, cnt):
		self.start = start
		self.end = end
		self.cnt = cnt

if __name__ == '__main__':
	app.run(debug=True,host="0.0.0.0",port=5000)

