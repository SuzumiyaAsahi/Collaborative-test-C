#include "poller.h"
#include <iostream>
#include <unistd.h>

int main(int argc, char *argv[]) {
  // 创建调度器
  Poller poller;
  // 注册第一个Future
  poller.Async(
      []() {
        int i = 0;
        while (1) {
          i++;
          std::cout << "锄禾日当午"
                    << " " << i << std::endl;
          usleep(500000);
        }
      },
      1);

  // 注册第二个Future
  poller.Async(
      []() {
        int i = 0;
        while (1) {
          i++;
          std::cout << "汗滴禾下土"
                    << " " << i << std::endl;
          usleep(500000);
        }
      },
      2);

  // 注册第三个Future
  poller.Async(
      []() {
        int i = 0;
        while (1) {
          i++;
          std::cout << "谁知盘中餐"
                    << " " << i << std::endl;
          usleep(500000);
        }
      },
      3);

  // 执行调度
  poller.Await();
  return 0;
}
