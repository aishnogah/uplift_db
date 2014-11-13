#include <cstdio>
#include <string>
#include <random>
#include <iostream>
#include <algorithm>

#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/options.h"
#include "rocksdb/statistics.h"
#include "rocksdb/perf_context.h"
#include "rocksdb/utilities/spatial_db.h"
#include "rocksdb/statistics.h"
#include "rocksdb/env.h"
#include "rocksdb/perf_context.h"


using namespace rocksdb;
using namespace spatial;

using std::pair;
using std::vector;
using std::cout;
using std::endl;

std::string kDBPath = "/Volumes/RAM Disk/db";
uint32_t kN = 1000000;
uint32_t kZoomLevels = 10;
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<> uniform(0,100);
bool kRamDisk = true;


void RandomCoords(int n, vector<Coord<double>> *result) {
  result->clear();
  for (int i=0; i<n; ++i) {
    double x = uniform(gen);
    double y = uniform(gen);
    result->push_back(Coord<double>(x, y));
  }
}

void RandomBoxes(int n,
        std::uniform_real_distribution<> dist,
        vector<BoundingBox<double>> *result) {
  result->clear();
  for (int i=0; i<n; ++i) {
    double minx = uniform(gen);
    double miny = uniform(gen);
    double maxx = std::min(100.0, dist(gen) + minx);
    double maxy = std::min(100.0, dist(gen) + miny);
    result->push_back(BoundingBox<double>(minx, miny, maxx, maxy));
  }
}

DBOptions GetDBOptions(const SpatialDBOptions& options) {
  DBOptions db_options;
  db_options.IncreaseParallelism(3);
  db_options.statistics = rocksdb::CreateDBStatistics();
  if (options.bulk_load) {
    db_options.stats_dump_period_sec = 600;
    db_options.disableDataSync = true;
  } else {
    db_options.stats_dump_period_sec = 1800;  // 30min
  }
  return db_options;
}

int main() {
  DestroyDB(kDBPath, Options());

  const BoundingBox<double> outer_box(0, 0, 100, 100);
  vector<SpatialIndexOptions> spatial_indexes;
  vector<std::string> si_names;

  SpatialDBOptions options;
  DBOptions db_options = GetDBOptions(options);
  db_options.allow_mmap_writes = true;
  db_options.allow_mmap_reads = true;

//  SetPerfLevel(kEnableTime);

  rocksdb::Status s = SpatialDB::Create(db_options, kDBPath, outer_box, kZoomLevels, kRamDisk);
  cout << s.ToString() << endl;
  assert(s.ok());

  SpatialDB* db;
  s = SpatialDB::Open(db_options, kDBPath, &db, false, kZoomLevels, kRamDisk);
  cout << s.ToString() << endl;
  assert(s.ok());
  cout << "created DB" << endl;

  vector<Coord<double>> data;
  RandomCoords(kN, &data);
  cout << "generated data" << endl;

  vector<rocksdb::Slice> blobs(kN, "blob");
  vector<FeatureSet> feature_sets(kN);
  rocksdb::WriteOptions write_options;
  write_options.disableWAL = true;
  s = db->BulkInsert(write_options, data, blobs, feature_sets);
  assert(s.ok());
  cout << "inserted data" << endl;
  db->Compact();
    
  return 0;
}