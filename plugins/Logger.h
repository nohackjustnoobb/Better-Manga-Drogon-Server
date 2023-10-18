/**
 *
 *  Logger.h
 *
 */

#pragma once

#include <drogon/plugins/Plugin.h>


class Logger : public drogon::Plugin<Logger>
{
  public:
    Logger() {}
    /// This method must be called by drogon to initialize and start the plugin.
    /// It must be implemented by the user.
    void initAndStart(const Json::Value &config) override;

    /// This method must be called by drogon to shutdown the plugin.
    /// It must be implemented by the user.
    void shutdown() override;
};

