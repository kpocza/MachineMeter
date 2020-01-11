#!/usr/bin/python
import os
from flask import Flask, request
from datetime import datetime, timedelta

app = Flask(__name__)

@app.route('/info', methods=['GET'])
def info():
    if os.path.exists("endtime.txt"):
        f = open("endtime.txt", "r")
        endSeconds = float(f.read())
        f.close()

        return "1" if totalSeconds(datetime.utcnow()) < endSeconds else "0"
    else:
        return "0"

@app.route('/short', methods=['GET'])
def shortCirculation():
	setEnd(15)
	return "OK"

@app.route('/long', methods=['GET'])
def longCirculation():
	setEnd(30)
	return "OK"

def setEnd(minutes):
	endTime = datetime.utcnow() + timedelta(minutes = minutes)
        f = open("endtime.txt", "w")
        f.write(str(totalSeconds(endTime)))
        f.close()

def totalSeconds(d):
    return (d - datetime(1,1,1)).total_seconds()

if __name__ == '__main__':
	app.run(debug=True,host="0.0.0.0",port=5001)

