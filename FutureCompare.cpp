#pragma once
#include "poller.h"
bool FutureCompare::operator()(const Future *left, const Future *right) const {
  if (left->priority == right->priority) {
    return left->sequence > right->sequence;
  }
  return left->priority < right->priority;
}
