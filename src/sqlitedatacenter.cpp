#include "sqlitedatacenter.h"




#ifdef hc_debug
#undef hc_debug
#define hc_debug printf
#else
#define hc_debug printf
#endif

SqliteDataCenter::SqliteDataCenter() {
  system_db = nullptr;
  exit = false;
}

SqliteDataCenter::SqliteDataCenter(const std::string &name,
                                   const std::string &table) {
  InitSystemSettingTable(name, table);
}

SqliteDataCenter::~SqliteDataCenter() {
  exit = true;
  sql_thread->join();
  if (system_db != nullptr) {
    sqlite3_close(system_db);
  }
}

int systemSettingCallback(void *para, int nr, char **values, char **names) {
  if (nr != 3) {
    return -1;
  }
  SqliteDataCenter *setting_center = (SqliteDataCenter *)para;
  SqliteDataCenter::SYSTEM_SETTING temp;
  std::string key = values[0];
  std::string group = values[1];
  std::string value = values[2];
  setting_center->system_setting[group][key] = value;
  return 0;
}

bool SqliteDataCenter::ExecSql(const std::string &sql,
                               sqlite3_callback callback, void *param) {
  char *err;
  if (sqlite3_exec(system_db, "begin transaction", 0, 0, &err) !=
      SQLITE_OK) //开始事务
  {
    hc_debug("error: %s\n", err);
    sqlite3_free(err);
    return false;
  }
  if (sqlite3_exec(system_db, sql.data(), callback, param, &err) != SQLITE_OK) {
    hc_debug("error: %s\n", err);
    sqlite3_free(err);
    return false;
  }
  if (sqlite3_exec(system_db, "commit transaction", 0, 0, &err) !=
      SQLITE_OK) //结束事务
  {
    hc_debug("error: %s\n", err);
    sqlite3_free(err);
    return false;
  }
  return true;
}

void *SqliteDataCenter::SqliteDataCenter::SqlThread(void *arg) {
  struct itimerspec ts;
  sql_tfd = timerfd_create(CLOCK_MONOTONIC, 0);
  if (sql_tfd == -1) {
    hc_debug("timerfd_create() failed: errno=%d\n", errno);
    return 0;
  }

  ts.it_interval.tv_sec = 0;
  ts.it_interval.tv_nsec = 1000000L;
  ts.it_value.tv_sec = 1;
  ts.it_value.tv_nsec = 0;

  if (timerfd_settime(sql_tfd, 0, &ts, NULL) < 0) {
    hc_debug("timerfd_settime() failed: errno=%d\n", errno);
    close(sql_tfd);
    return 0;
  }
  while (1) {
    read(sql_tfd, &sql_exp, sizeof(uint64_t));

    if (sql_queue.size_approx() == 0) {
      if (exit) {
        break;
      }
      continue;
    }
    char *err;
    if (sqlite3_exec(system_db, "begin transaction", 0, 0, &err) !=
        SQLITE_OK) //开始事务
    {
      hc_debug("error: %s\n", err);
      sqlite3_free(err);
      continue;
    }
    std::string sql;
    while (sql_queue.try_dequeue(sql)) {
      if (sqlite3_exec(system_db, sql.data(), nullptr, nullptr, &err) !=
          SQLITE_OK) {
        hc_debug("error: %s\n", err);
        sqlite3_free(err);
        break;
      }
    }
    if (sqlite3_exec(system_db, "commit transaction", 0, 0, &err) !=
        SQLITE_OK) //结束事务
    {
      hc_debug("error: %s\n", err);
      sqlite3_free(err);
      continue;
    }
  }
  return 0;
}

//< 初始化表
bool SqliteDataCenter::InitSystemSettingTable(const std::string &name,
                                              const std::string &table) {
  {
    table_name = table;
    if (sqlite3_open(name.data(), &system_db)) {
      hc_debug("Can't open database:%s\n", sqlite3_errmsg(system_db));
      sqlite3_close(system_db);
      system_db = nullptr;
      return false;
    }
    std::string sql;
    sql = "CREATE TABLE  IF NOT EXISTS \"";
    sql += table;
    sql += "\" (\"key_\" TEXT,\"group_\"  TEXT, \"value_\" TEXT,PRIMARY "
           "KEY(\"key_\",\"group_\"));";

    ExecSql(sql);
  }
  {
    char sql[1024] = {0};
    sprintf(sql, "SELECT * FROM %s ", table.data());
    ExecSql(sql, systemSettingCallback, &system_setting);
  }

  sql_thread = new std::thread(&SqliteDataCenter::SqlThread, this, nullptr);
  return true;
}

bool SqliteDataCenter::InsertSystemSettingTable(const std::string &key,
                                                const std::string &group,
                                                const std::string &value) {
  char sql[1024] = {0};
  sprintf(sql, "INSERT INTO %s VALUES(\'%s\', \'%s\', \'%s\')",
          table_name.data(), key.data(), group.data(), value.data());
  sql_queue.enqueue(sql);
  return true;
}

bool SqliteDataCenter::UpdateSystemSettingTable(const std::string &key,
                                                const std::string &group,
                                                const std::string &value) {
  char sql[1024] = {0};
  sprintf(sql, "UPDATE %s SET key_=\'%s\', group_=\'%s\', value_=\'%s\'WHERE "
               "key_=\'%s\' AND group_ = \'%s\'')",
          table_name.data(), key.data(), group.data(), value.data(), key.data(),
          group.data());
  sql_queue.enqueue(sql);
  return true;
}

bool SqliteDataCenter::DeleteSystemSettingTable(const std::string &key,
                                                const std::string &group) {
  char sql[1024] = {0};
  sprintf(sql, "DELETE FROM %s WHERE key_=\'%s\'AND  group_ = \'%s\'",
          table_name.data(), key.data(), group.data());
  sql_queue.enqueue(sql);
  return true;
}

std::string SqliteDataCenter::GetCustomSettings(const std::string &key,
                                                const std::string &defval,
                                                const std::string &group) {
  if (system_setting.find(group) != system_setting.end()) {
    if (system_setting[group].find(key) != system_setting[group].end()) {
      return system_setting[group][key];
    }
  }
  system_setting[group][key] = defval;
  InsertSystemSettingTable(key, group, defval);
  return defval;
}

void SqliteDataCenter::SetSystemSetting(const std::string &key,
                                        const std::string &value,
                                        const std::string &group) {
  bool is_exist = false;
  if (system_setting.find(group) != system_setting.end()) {
    if (system_setting[group].find(key) != system_setting[group].end()) {
      is_exist = true;
      if (system_setting[group][key] == value) {
        return;
      }
    }
  }
  system_setting[group][key] = value;
  if (is_exist) {
    UpdateSystemSettingTable(key, group, value);
  } else {
    InsertSystemSettingTable(key, group, value);
  }
}
