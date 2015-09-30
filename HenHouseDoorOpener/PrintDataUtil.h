/*
 * PrintDataUtil.h
 *
 *  Created on: 30/09/2015
 *      Author: jabkj
 */

#ifndef PRINTDATAUTIL_H_
#define PRINTDATAUTIL_H_

//Print an integer in "00" format (with leading zero),
//followed by a delimiter character to Serial.
//Input value assumed to be between 0 and 99.
void printI00(int val, char delim)
{
    if (val < 10) Serial << '0';
    Serial << _DEC(val);
    if (delim > 0) Serial << delim;
    return;
}

//print time to Serial
void printTime(time_t t)
{
    printI00(hour(t), ':');
    printI00(minute(t), ':');
    printI00(second(t), ' ');
}

//print date to Serial
void printDate(time_t t)
{
    printI00(day(t), 0);
    Serial << monthShortStr(month(t)) << _DEC(year(t));
}

//print date and time to Serial
void printDateTime(time_t t)
{
#ifdef JABK_SERIAL_DEBUG
	printDate(t);
	Serial << ' ';
	printTime(t);
	Serial << endl;
#endif
}



#endif /* PRINTDATAUTIL_H_ */
