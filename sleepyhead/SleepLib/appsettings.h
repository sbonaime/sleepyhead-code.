/* SleepLib AppSettings Header
 *
 * This file for all settings related stuff to clean up Preferences & Profiles.
 *
 * Copyright (c) 2011-2018 Mark Watkins <mark@jedimark.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the sourcecode. */

#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QDateTime>
#include "preferences.h"
#include "common.h"
#include "version.h"


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

const QString STR_GEN_AutoOpenLastUsed = "AutoOpenLastUsed";
const QString STR_GEN_UpdatesLastChecked = "UpdatesLastChecked";
const QString STR_GEN_UpdatesAutoCheck = "Updates_AutoCheck";
const QString STR_GEN_UpdateCheckFrequency = "Updates_CheckFrequency";
const QString STR_GEN_Language = "Language";
const QString STR_PREF_AllowEarlyUpdates = "AllowEarlyUpdates";
const QString STR_PREF_VersionString = "VersionString";
const QString STR_GEN_ShowAboutDialog = "ShowAboutDialog";


class AppWideSetting: public PrefSettings
{
public:
  AppWideSetting(Preferences *pref)
    : PrefSettings(pref)
  {
      m_multithreading = initPref(STR_IS_Multithreading, idealThreads() > 1).toBool();
      initPref(STR_US_ShowPerformance, false);
      initPref(STR_US_ShowDebug, false);
      initPref(STR_AS_CalendarVisible, true);
      m_scrollDampening = initPref(STR_US_ScrollDampening, (int)50).toInt();
      m_tooltipTimeout = initPref(STR_US_TooltipTimeout, (int)2500).toInt();
      m_graphHeight=initPref(STR_AS_GraphHeight, 180).toInt();
      initPref(STR_AS_DailyPanelWidth, 350.0);
      initPref(STR_AS_RightPanelWidth, 230.0);
      m_antiAliasing=initPref(STR_AS_AntiAliasing, true).toBool();
      initPref(STR_AS_GraphSnapshots, true);
      initPref(STR_AS_Animations, true);
      m_squareWavePlots = initPref(STR_AS_SquareWave, false).toBool();
      initPref(STR_AS_AllowYAxisScaling, true);
      initPref(STR_AS_GraphTooltips, true);
      m_usePixmapCaching = initPref(STR_AS_UsePixmapCaching, false).toBool();
      initPref(STR_AS_OverlayType, ODT_Bars);
      initPref(STR_AS_OverviewLinechartMode, OLC_Bartop);
      m_lineThickness=initPref(STR_AS_LineThickness, 1.0).toFloat();
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
      initPref(STR_GEN_AutoOpenLastUsed, true);

      initPref(STR_GEN_UpdatesAutoCheck, true);
      initPref(STR_GEN_UpdateCheckFrequency, 7);
      initPref(STR_PREF_AllowEarlyUpdates, false);
      initPref(STR_GEN_UpdatesLastChecked, QDateTime());
      initPref(STR_PREF_VersionString, VersionString);
      initPref(STR_GEN_Language, "en_US");
      initPref(STR_GEN_ShowAboutDialog, 0);  // default to about screen, set to -1 afterwards
  }

  bool m_usePixmapCaching, m_antiAliasing, m_squareWavePlots;
  int m_tooltipTimeout, m_graphHeight, m_scrollDampening;
  bool m_multithreading;
  float m_lineThickness;

  QString versionString() const { return getPref(STR_PREF_VersionString).toString(); }
  bool updatesAutoCheck() const { return getPref(STR_GEN_UpdatesAutoCheck).toBool(); }
  bool allowEarlyUpdates() const { return getPref(STR_PREF_AllowEarlyUpdates).toBool(); }
  QDateTime updatesLastChecked() const { return getPref(STR_GEN_UpdatesLastChecked).toDateTime(); }
  int updateCheckFrequency() const { return getPref(STR_GEN_UpdateCheckFrequency).toInt(); }
  int showAboutDialog() const { return getPref(STR_GEN_ShowAboutDialog).toInt(); }
  void setShowAboutDialog(int tab) {setPref(STR_GEN_ShowAboutDialog, tab); }

  QString profileName() const { return getPref(STR_GEN_Profile).toString(); }
  bool autoLaunchImport() const { return getPref(STR_US_AutoLaunchImport).toBool(); }
  bool cacheSessions() const { return getPref(STR_IS_CacheSessions).toBool(); }
  bool multithreading() const { return m_multithreading; }
  bool showDebug() const { return getPref(STR_US_ShowDebug).toBool(); }
  bool showPerformance() const { return getPref(STR_US_ShowPerformance).toBool(); }
  //! \brief Whether to show the calendar
  bool calendarVisible() const { return getPref(STR_AS_CalendarVisible).toBool(); }
  int scrollDampening() const { return m_scrollDampening; }
  int tooltipTimeout() const { return m_tooltipTimeout; }
  //! \brief Returns the normal (unscaled) height of a graph
  int graphHeight() const { return m_graphHeight; }
  //! \brief Returns the normal (unscaled) height of a graph
  int dailyPanelWidth() const { return getPref(STR_AS_DailyPanelWidth).toInt(); }
  //! \brief Returns the normal (unscaled) height of a graph
  int rightPanelWidth() const { return getPref(STR_AS_RightPanelWidth).toInt(); }
  //! \brief Returns true if AntiAliasing (the graphical smoothing method) is enabled
  bool antiAliasing() const { return m_antiAliasing; }
  //! \brief Returns true if renderPixmap function is in use, which takes snapshots of graphs
  bool graphSnapshots() const { return getPref(STR_AS_GraphSnapshots).toBool(); }
  //! \brief Returns true if Graphical animations & Transitions will be drawn
  bool animations() const { return getPref(STR_AS_Animations).toBool(); }
  //! \brief Returns true if PixmapCaching acceleration will be used
  inline const bool & usePixmapCaching() const { return m_usePixmapCaching; }
  //! \brief Returns true if Square Wave plots are preferred (where possible)
  bool squareWavePlots() const { return m_squareWavePlots; }
  //! \brief Whether to allow double clicking on Y-Axis labels to change vertical scaling mode
  bool allowYAxisScaling() const { return getPref(STR_AS_AllowYAxisScaling).toBool(); }
  //! \brief Whether to show graph tooltips
  bool graphTooltips() const { return getPref(STR_AS_GraphTooltips).toBool(); }
  //! \brief Pen width of line plots
  float lineThickness() const { return m_lineThickness; }
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
  bool autoOpenLastUsed() const { return getPref(STR_GEN_AutoOpenLastUsed).toBool(); }
  QString language() const { return getPref(STR_GEN_Language).toString(); }

  void setProfileName(QString name) { setPref(STR_GEN_Profile, name); }
  void setAutoLaunchImport(bool b) { setPref(STR_US_AutoLaunchImport, b); }
  void setCacheSessions(bool c) { setPref(STR_IS_CacheSessions, c); }
  void setMultithreading(bool b) { setPref(STR_IS_Multithreading, m_multithreading = b); }
  void setShowDebug(bool b) { setPref(STR_US_ShowDebug, b); }
  void setShowPerformance(bool b) { setPref(STR_US_ShowPerformance, b); }
  //! \brief Sets whether to display the (Daily View) Calendar
  void setCalendarVisible(bool b) { setPref(STR_AS_CalendarVisible, b); }
  void setScrollDampening(int i) { setPref(STR_US_ScrollDampening, m_scrollDampening=i); }
  void setTooltipTimeout(int i) { setPref(STR_US_TooltipTimeout, m_tooltipTimeout=i); }
  //! \brief Set the normal (unscaled) height of a graph.
  void setGraphHeight(int height) { setPref(STR_AS_GraphHeight, m_graphHeight=height); }
  //! \brief Set the normal (unscaled) height of a graph.
  void setDailyPanelWidth(int width) { setPref(STR_AS_DailyPanelWidth, width); }
  //! \brief Set the normal (unscaled) height of a graph.
  void setRightPanelWidth(int width) { setPref(STR_AS_RightPanelWidth, width); }
  //! \brief Set to true to turn on AntiAliasing (the graphical smoothing method)
  void setAntiAliasing(bool aa) { setPref(STR_AS_AntiAliasing, m_antiAliasing=aa); }
  //! \brief Set to true if renderPixmap functions are in use, which takes snapshots of graphs.
  void setGraphSnapshots(bool gs) { setPref(STR_AS_GraphSnapshots, gs); }
  //! \brief Set to true if Graphical animations & Transitions will be drawn
  void setAnimations(bool anim) { setPref(STR_AS_Animations, anim); }
  //! \brief Set to true to use Pixmap Caching of Text and other graphics caching speedup techniques
  void setUsePixmapCaching(bool b) { setPref(STR_AS_UsePixmapCaching, m_usePixmapCaching=b); }
  //! \brief Set whether or not to useSquare Wave plots (where possible)
  void setSquareWavePlots(bool sw) { setPref(STR_AS_SquareWave, m_squareWavePlots=sw); }
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
  void setLineThickness(float size) { setPref(STR_AS_LineThickness, m_lineThickness=size); }
  //! \brief Sets whether to display Line Cursor
  void setLineCursorMode(bool b) { setPref(STR_AS_LineCursorMode, b); }
  //! \brief Sets whether to display the right sidebar
  void setRightSidebarVisible(bool b) { setPref(STR_AS_RightSidebarVisible, b); }
  void setUserEventPieChart(bool b) { setPref(STR_CS_UserEventPieChart, b); }
  void setShowSerialNumbers(bool enabled) { setPref(STR_US_ShowSerialNumbers, enabled); }
  void setOpenTabAtStart(int idx) { setPref(STR_US_OpenTabAtStart, idx); }
  void setOpenTabAfterImport(int idx) { setPref(STR_US_OpenTabAfterImport, idx); }
  void setRemoveCardReminder(bool b) { setPref(STR_US_RemoveCardReminder, b); }

  void setVersionString(QString version) { setPref(STR_PREF_VersionString, version); }
  void setUpdatesAutoCheck(bool b) { setPref(STR_GEN_UpdatesAutoCheck, b); }
  void setAllowEarlyUpdates(bool b)  { setPref(STR_PREF_AllowEarlyUpdates, b); }
  void setUpdatesLastChecked(QDateTime datetime) { setPref(STR_GEN_UpdatesLastChecked, datetime); }
  void setUpdateCheckFrequency(int freq) { setPref(STR_GEN_UpdateCheckFrequency,freq); }
  void setAutoOpenLastUsed(bool b) { setPref(STR_GEN_AutoOpenLastUsed , b); }
  void setLanguage(QString language) { setPref(STR_GEN_Language, language); }

};


extern AppWideSetting *AppSetting;



#endif // APPSETTINGS_H
