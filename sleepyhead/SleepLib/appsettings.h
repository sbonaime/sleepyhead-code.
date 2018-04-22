/* SleepLib AppSettings Header
 *
 * This file for all settings related stuff to clean up Preferences & Profiles.
 *
 * Copyright (c) 2011-2018 Mark Watkins <mark@jedimark.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include "preferences.h"
#include "common.h"


class Preferences;

enum OverviewLinechartModes { OLC_Bartop, OLC_Lines };


// ApplicationWideSettings Strings
const QString STR_CS_UserEventPieChart = "UserEventPieChart";
const QString STR_IS_Multithreading = "EnableMultithreading";
const QString STR_AS_GraphHeight = "GraphHeight";
const QString STR_AS_DailyPanelWidth = "DailyPanelWidth";
const QString STR_AS_RightPanelWidth = "RightPanelWidth";
const QString STR_AS_AntiAliasing = "UseAntiAliasing";
const QString STR_AS_GraphSnapshots = "EnableGraphSnapshots";
const QString STR_AS_Animations = "AnimationsAndTransitions";
const QString STR_AS_SquareWave = "SquareWavePlots";
const QString STR_AS_OverlayType = "OverlayType";
const QString STR_AS_OverviewLinechartMode = "OverviewLinechartMode";
const QString STR_AS_UsePixmapCaching = "UsePixmapCaching";
const QString STR_AS_AllowYAxisScaling = "AllowYAxisScaling";
const QString STR_AS_GraphTooltips = "GraphTooltips";
const QString STR_AS_LineThickness = "LineThickness";
const QString STR_AS_LineCursorMode = "LineCursorMode";
const QString STR_AS_CalendarVisible = "CalendarVisible";
const QString STR_AS_RightSidebarVisible = "RightSidebarVisible";
const QString STR_US_TooltipTimeout = "TooltipTimeout";
const QString STR_US_ScrollDampening = "ScrollDampening";
const QString STR_US_ShowDebug = "ShowDebug";
const QString STR_US_ShowPerformance = "ShowPerformance";
const QString STR_US_ShowSerialNumbers = "ShowSerialNumbers";
const QString STR_US_OpenTabAtStart = "OpenTabAtStart";
const QString STR_US_OpenTabAfterImport = "OpenTabAfterImport";
const QString STR_US_AutoLaunchImport = "AutoLaunchImport";
const QString STR_US_RemoveCardReminder = "RemoveCardReminder";
const QString STR_IS_CacheSessions = "MemoryHog";


class AppWideSetting: public PrefSettings
{
public:
  AppWideSetting(Preferences *pref)
    : PrefSettings(pref)
  {
      initPref(STR_IS_Multithreading, idealThreads() > 1);
      initPref(STR_US_ShowPerformance, false);
      initPref(STR_US_ShowDebug, false);
      initPref(STR_AS_CalendarVisible, true);
      initPref(STR_US_ScrollDampening, (int)50);
      initPref(STR_US_TooltipTimeout, (int)2500);
      initPref(STR_AS_GraphHeight, 180.0);
      initPref(STR_AS_DailyPanelWidth, 350.0);
      initPref(STR_AS_RightPanelWidth, 230.0);
      initPref(STR_AS_AntiAliasing, true);
      initPref(STR_AS_GraphSnapshots, true);
      initPref(STR_AS_Animations, true);
      initPref(STR_AS_SquareWave, false);
      initPref(STR_AS_AllowYAxisScaling, true);
      initPref(STR_AS_GraphTooltips, true);
      initPref(STR_AS_UsePixmapCaching, false);
      initPref(STR_AS_OverlayType, ODT_Bars);
      initPref(STR_AS_OverviewLinechartMode, OLC_Bartop);
      initPref(STR_AS_LineThickness, 1.0);
      initPref(STR_AS_LineCursorMode, true);
      initPref(STR_AS_RightSidebarVisible, true);
      initPref(STR_CS_UserEventPieChart, false);
      initPref(STR_US_ShowSerialNumbers, false);
      initPref(STR_US_OpenTabAtStart, 1);
      initPref(STR_US_OpenTabAfterImport, 0);
      initPref(STR_US_AutoLaunchImport, false);
      initPref(STR_IS_CacheSessions, false);
      initPref(STR_US_RemoveCardReminder, true);
      initPref(STR_GEN_Profile, "");
  }

  QString profileName() const { return getPref(STR_GEN_Profile).toString(); }
  bool autoLaunchImport() const { return getPref(STR_US_AutoLaunchImport).toBool(); }
  bool cacheSessions() const { return getPref(STR_IS_CacheSessions).toBool(); }
  bool multithreading() const { return getPref(STR_IS_Multithreading).toBool(); }
  bool showDebug() const { return getPref(STR_US_ShowDebug).toBool(); }
  bool showPerformance() const { return getPref(STR_US_ShowPerformance).toBool(); }
  //! \brief Whether to show the calendar
  bool calendarVisible() const { return getPref(STR_AS_CalendarVisible).toBool(); }
  int scrollDampening() const { return getPref(STR_US_ScrollDampening).toInt(); }
  int tooltipTimeout() const { return getPref(STR_US_TooltipTimeout).toInt(); }
  //! \brief Returns the normal (unscaled) height of a graph
  int graphHeight() const { return getPref(STR_AS_GraphHeight).toInt(); }
  //! \brief Returns the normal (unscaled) height of a graph
  int dailyPanelWidth() const { return getPref(STR_AS_DailyPanelWidth).toInt(); }
  //! \brief Returns the normal (unscaled) height of a graph
  int rightPanelWidth() const { return getPref(STR_AS_RightPanelWidth).toInt(); }
  //! \brief Returns true if AntiAliasing (the graphical smoothing method) is enabled
  bool antiAliasing() const { return getPref(STR_AS_AntiAliasing).toBool(); }
  //! \brief Returns true if renderPixmap function is in use, which takes snapshots of graphs
  bool graphSnapshots() const { return getPref(STR_AS_GraphSnapshots).toBool(); }
  //! \brief Returns true if Graphical animations & Transitions will be drawn
  bool animations() const { return getPref(STR_AS_Animations).toBool(); }
  //! \brief Returns true if PixmapCaching acceleration will be used
  bool usePixmapCaching() const { return getPref(STR_AS_UsePixmapCaching).toBool(); }
  //! \brief Returns true if Square Wave plots are preferred (where possible)
  bool squareWavePlots() const { return getPref(STR_AS_SquareWave).toBool(); }
  //! \brief Whether to allow double clicking on Y-Axis labels to change vertical scaling mode
  bool allowYAxisScaling() const { return getPref(STR_AS_AllowYAxisScaling).toBool(); }
  //! \brief Whether to show graph tooltips
  bool graphTooltips() const { return getPref(STR_AS_GraphTooltips).toBool(); }
  //! \brief Pen width of line plots
  float lineThickness() const { return getPref(STR_AS_LineThickness).toFloat(); }
  //! \brief Whether to show line cursor
  bool lineCursorMode() const { return getPref(STR_AS_LineCursorMode).toBool(); }
  //! \brief Whether to show the right sidebar
  bool rightSidebarVisible() const { return getPref(STR_AS_RightSidebarVisible).toBool(); }
  //! \brief Returns the type of overlay flags (which are displayed over the Flow Waveform)
  OverlayDisplayType overlayType() const {
      return (OverlayDisplayType)getPref(STR_AS_OverlayType).toInt();
  }
  //! \brief Returns the display type of Overview pages linechart
  OverviewLinechartModes overviewLinechartMode() const {
      return (OverviewLinechartModes)getPref(STR_AS_OverviewLinechartMode).toInt();
  }
  bool userEventPieChart() const { return getPref(STR_CS_UserEventPieChart).toBool(); }
  bool showSerialNumbers() const { return getPref(STR_US_ShowSerialNumbers).toBool(); }
  int openTabAtStart() const { return getPref(STR_US_OpenTabAtStart).toInt(); }
  int openTabAfterImport() const { return getPref(STR_US_OpenTabAfterImport).toInt(); }
  bool removeCardReminder() const { return getPref(STR_US_RemoveCardReminder).toBool(); }


  void setProfileName(QString name) { setPref(STR_GEN_Profile, name); }

  void setAutoLaunchImport(bool b) { setPref(STR_US_AutoLaunchImport, b); }
  void setCacheSessions(bool c) { setPref(STR_IS_CacheSessions, c); }
  void setMultithreading(bool enabled) { setPref(STR_IS_Multithreading, enabled); }
  void setShowDebug(bool b) { setPref(STR_US_ShowDebug, b); }
  void setShowPerformance(bool b) { setPref(STR_US_ShowPerformance, b); }
  //! \brief Sets whether to display the (Daily View) Calendar
  void setCalendarVisible(bool b) { setPref(STR_AS_CalendarVisible, b); }
  void setScrollDampening(int i) { setPref(STR_US_ScrollDampening, i); }
  void setTooltipTimeout(int i) { setPref(STR_US_TooltipTimeout, i); }
  //! \brief Set the normal (unscaled) height of a graph.
  void setGraphHeight(int height) { setPref(STR_AS_GraphHeight, height); }
  //! \brief Set the normal (unscaled) height of a graph.
  void setDailyPanelWidth(int width) { setPref(STR_AS_DailyPanelWidth, width); }
  //! \brief Set the normal (unscaled) height of a graph.
  void setRightPanelWidth(int width) { setPref(STR_AS_RightPanelWidth, width); }
  //! \brief Set to true to turn on AntiAliasing (the graphical smoothing method)
  void setAntiAliasing(bool aa) { setPref(STR_AS_AntiAliasing, aa); }
  //! \brief Set to true if renderPixmap functions are in use, which takes snapshots of graphs.
  void setGraphSnapshots(bool gs) { setPref(STR_AS_GraphSnapshots, gs); }
  //! \brief Set to true if Graphical animations & Transitions will be drawn
  void setAnimations(bool anim) { setPref(STR_AS_Animations, anim); }
  //! \brief Set to true to use Pixmap Caching of Text and other graphics caching speedup techniques
  void setUsePixmapCaching(bool b) { setPref(STR_AS_UsePixmapCaching, b); }
  //! \brief Set whether or not to useSquare Wave plots (where possible)
  void setSquareWavePlots(bool sw) { setPref(STR_AS_SquareWave, sw); }
  //! \brief Sets the type of overlay flags (which are displayed over the Flow Waveform)
  void setOverlayType(OverlayDisplayType od) { setPref(STR_AS_OverlayType, (int)od); }
  //! \brief Sets whether to allow double clicking on Y-Axis labels to change vertical scaling mode
  void setAllowYAxisScaling(bool b) { setPref(STR_AS_AllowYAxisScaling, b); }
  //! \brief Sets whether to allow double clicking on Y-Axis labels to change vertical scaling mode
  void setGraphTooltips(bool b) { setPref(STR_AS_GraphTooltips, b); }
  //! \brief Sets the type of overlay flags (which are displayed over the Flow Waveform)
  void setOverviewLinechartMode(OverviewLinechartModes od) {
      setPref(STR_AS_OverviewLinechartMode, (int)od);
  }
  //! \brief Set the pen width of line plots.
  void setLineThickness(float size) { setPref(STR_AS_LineThickness, size); }
  //! \brief Sets whether to display Line Cursor
  void setLineCursorMode(bool b) { setPref(STR_AS_LineCursorMode, b); }
  //! \brief Sets whether to display the right sidebar
  void setRightSidebarVisible(bool b) { setPref(STR_AS_RightSidebarVisible, b); }
  void setUserEventPieChart(bool b) { setPref(STR_CS_UserEventPieChart, b); }
  void setShowSerialNumbers(bool enabled) { setPref(STR_US_ShowSerialNumbers, enabled); }
  void setOpenTabAtStart(int idx) { setPref(STR_US_OpenTabAtStart, idx); }
  void setOpenTabAfterImport(int idx) { setPref(STR_US_OpenTabAfterImport, idx); }
  void setRemoveCardReminder(bool b) { setPref(STR_US_RemoveCardReminder, b); }
};


extern AppWideSetting *AppSetting;



#endif // APPSETTINGS_H
