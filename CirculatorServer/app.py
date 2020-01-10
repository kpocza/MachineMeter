#!/usr/bin/python
from flask import Flask, request
from datetime import datetime, timedelta

app = Flask(__name__)

endTime = datetime.utcnow()

@app.route('/info', methods=['GET'])
def info():
	global endTime
	return "1" if datetime.utcnow() < endTime else "0"

@app.route('/short', methods=['GET'])
def shortCirculation():
	setEnd(15)
	return "OK"

@app.route('/long', methods=['GET'])
def longCirculation():
	setEnd(30)
	return "OK"

def setEnd(minutes):
	global endTime
	endTime = datetime.utcnow() + timedelta(minutes = minutes)

if __name__ == '__main__':
	app.run(debug=True,host="0.0.0.0",port=5001)

