#ifndef SQLITEDATACENTER_H
#define SQLITEDATACENTER_H

#include "concurrentqueue.h"
#include "sqlite3.h"
#include "string"
#include <map>
#include <pthread.h>
#include <sys/timerfd.h>
#include <thread>
#include <unistd.h>

class SqliteDataCenter {
public:
  SqliteDataCenter();
  SqliteDataCenter(const std::string &name, const std::string &table);
  ~SqliteDataCenter();

  typedef std::map<std::string, std::map<std::string, std::string>>
      SYSTEM_SETTING;

  friend int systemSettingCallback(void *para, int nr, char **values,
                                   char **names);
  bool InitSystemSettingTable(const std::string &name,
                              const std::string &table);

public:
  std::string GetCustomSettings(const std::string &key,
                                const std::string &defval,
                                const std::string &group = "custom");
  void SetSystemSetting(const std::string &key, const std::string &value,
                        const std::string &group = "custom");

private:
  bool InsertSystemSettingTable(const std::string &key,
                                const std::string &group,
                                const std::string &value);
  bool UpdateSystemSettingTable(const std::string &key,
                                const std::string &group,
                                const std::string &value);
  bool DeleteSystemSettingTable(const std::string &key,
                                const std::string &group);

  bool ExecSql(const std::string &sql, sqlite3_callback callback = nullptr,
               void *param = nullptr);
  void *SqlThread(void *arg);

private:
  std::string table_name;
  SYSTEM_SETTING system_setting;
  sqlite3 *system_db = nullptr;
  std::thread *sql_thread;
  bool exit;
  moodycamel::ConcurrentQueue<std::string> sql_queue;
  int sql_tfd;
  uint64_t sql_exp;
};

#endif // SQLITEDATACENTER_H
