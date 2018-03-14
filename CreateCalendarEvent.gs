function doGet(eve){
  var title = eve.parameter.title;
  Logger.log(eve);
 //Logger.log(ContentService.createTextOutput(CreateEvent()));
  return ContentService.createTextOutput(CreateEvent(title));
}

// Replace XXX with your calendar name
function CreateEvent(eve){
  var _calendarName = 'XXXs_Duties'
  var MILLIS_PER_HOUR = 1000 * 60 * 15 ;
  var now = new Date();
  var from = new Date(now.getTime()-60000);
  var to = new Date(now.getTime() + 1 * MILLIS_PER_HOUR);
  var vCalendar = CalendarApp.getCalendarsByName(_calendarName)[0];
   vCalendar.createEvent(eve,
                        from,
                        to)
// Logger.log(currentTime);
//  Logger.log(vStartDateValue);
// Logger.log(from);
// Logger.log(to);
//  Logger.log(vEndDateValue);
return true;
}