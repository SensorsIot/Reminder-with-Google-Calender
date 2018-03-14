function doGet(){
  // Logger.log(ContentService.createTextOutput(GetEvents()));
  return ContentService.createTextOutput(GetEvents());
}

// Replace XXX with your calendar name

function GetEvents(){
  var _calendarName = 'XXXs_Duties'
  var Cal = CalendarApp.getCalendarsByName(_calendarName)[0];
  var Now = new Date();
  var Later = new Date();
  Later.setSeconds(Now.getSeconds() + 1);
  // Logger.log(Now);
  Logger.log(Later);
  var events = Cal.getEvents(Now, Later);
  // Logger.log(events.length);
  str = "";
  for (var i = 0; i < events.length; i++){
    str += events[i].getTitle() + ';' ;
    //str += '\n';
  }
  str = str.substring(0, str.length - 1);
  // Logger.log(str);
  return str;
}