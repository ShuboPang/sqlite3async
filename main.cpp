#include "sqlitedatacenter.h"
#include <iostream>

using namespace std;

int main() {

  SqliteDataCenter *sqlite = new SqliteDataCenter();
  bool ret = sqlite->InitSystemSettingTable("EtherCAT", "tb_system_setting");
  if (ret) {
    cout << "Hello World!" << endl;
    sqlite->SetSystemSetting("demo", "3456789");
  }
  delete sqlite;
  return 0;
}
