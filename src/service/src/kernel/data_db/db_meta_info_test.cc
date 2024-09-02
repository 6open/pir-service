#include <gtest/gtest.h>
#include<iostream>
using namespace std;
#include "db_meta_info.h"
#define private public
// MegaBatch class tests
class DbMetaInfoTest : public ::testing::Test 
{
public:
    std::string key_;
    std::string meta_path_;
    std::vector<std::string> key_columns_ = {"id1","id2"};
    std::vector<std::string> label_columns_ = {"la1","la2"};
};

TEST_F(DbMetaInfoTest, TotalSizeTest)
{
    MegaBatch batch("Batch1", 100, 1);
    EXPECT_EQ(batch.TotalSize(), 100);
}

TEST_F(DbMetaInfoTest, LessThanOperatorTest)
{
    MegaBatch batch1("Batch1", 100, 1);
    MegaBatch batch2("Batch2", 200, 2);
    EXPECT_LT(batch1, batch2);
}

TEST_F(DbMetaInfoTest, GreaterThanOperatorTest)
{
    MegaBatch batch1("Batch1", 100, 1);
    MegaBatch batch2("Batch2", 200, 2);
    EXPECT_GT(batch2, batch1);
}

TEST_F(DbMetaInfoTest, PlusEqualsOperatorTest)
{
    MegaBatch batch1("Batch1", 100, 1);
    MegaBatch batch2("Batch2", 200, 2);
    batch1 += batch2;
    EXPECT_EQ(batch1.TotalSize(), 300);
    EXPECT_EQ(batch1.batch_names_.size(), 2);
    EXPECT_EQ(batch1.per_batch_size_.size(), 2);
    EXPECT_EQ(batch1.batch_count_.size(), 2);
}

TEST_F(DbMetaInfoTest, Serialization)
{
    // 创建一个MegaBatch对象并设置数据
    MegaBatch megabatch("bt_name", 100, 10);
    megabatch.self_num_ = 123;
    megabatch.batch_names_ = {"batch1", "batch2", "batch3"};
    megabatch.per_batch_size_ = {10, 20, 30};
    megabatch.batch_count_ = {1, 2, 3};

    // 定义序列化和反序列化后的对象
    MegaBatch deserializedMegabatch;

    // 序列化数据到文件
    std::string filename = "megabatch.dat";
    std::ofstream out(filename);
    megabatch.Serialize(out);
    out.close();

    // 从文件中反序列化生成对象
    std::ifstream in(filename);
    deserializedMegabatch = MegaBatch::Deserialize(in);
    in.close();

    // 验证反序列化后的对象与原始对象的数据一致性
    EXPECT_EQ(megabatch.total_size, deserializedMegabatch.total_size);
    EXPECT_EQ(megabatch.self_num_, deserializedMegabatch.self_num_);
    EXPECT_EQ(megabatch.batch_names_, deserializedMegabatch.batch_names_);
    EXPECT_EQ(megabatch.per_batch_size_, deserializedMegabatch.per_batch_size_);
    EXPECT_EQ(megabatch.batch_count_, deserializedMegabatch.batch_count_);
}


// DbMetaInfo class tests
TEST_F(DbMetaInfoTest, EqualOperator)
{
    std::vector<std::string> batch_names = {"Batch1", "Batch2", "Batch3"};
    std::vector<std::size_t> num_count = {100, 200, 300};

    DbMetaInfo dbMetaInfo(key_,meta_path_,key_columns_,label_columns_);
    dbMetaInfo.SetBatch(batch_names, num_count);
    DbMetaInfo dbMetaInfo2 = dbMetaInfo;
    EXPECT_EQ(dbMetaInfo,dbMetaInfo2);
    // dbMetaInfo2.key_ ="jjj";
    // EXPECT_EQ(dbMetaInfo,dbMetaInfo2);
}
TEST_F(DbMetaInfoTest, MergeBatchTest)
{
    std::vector<std::string> batch_names = {"Batch1", "Batch2", "Batch3"};
    std::vector<std::size_t> num_count = {100, 200, 300};

    DbMetaInfo dbMetaInfo(key_,meta_path_,key_columns_,label_columns_);
    dbMetaInfo.SetBatch(batch_names, num_count);

    EXPECT_EQ(dbMetaInfo.mega_batchs_.size(), 1);
    EXPECT_EQ(dbMetaInfo.mega_batchs_[0].TotalSize(), 600);
    EXPECT_EQ(dbMetaInfo.mega_batchs_[0].batch_names_.size(), 3);
    EXPECT_EQ(dbMetaInfo.mega_batchs_[0].per_batch_size_.size(), 3);
    EXPECT_EQ(dbMetaInfo.mega_batchs_[0].batch_count_.size(), 3);
}

TEST_F(DbMetaInfoTest, MergeBatchTest2)
{
    std::vector<std::string> batch_names = {"Batch1", "Batch2", "Batch3"};
    std::vector<std::size_t> num_count = {kPerBatchMinSize-100, kPerBatchMinSize+300, kPerBatchMinSize+200};

    DbMetaInfo dbMetaInfo(key_,meta_path_,key_columns_,label_columns_);
    dbMetaInfo.SetBatch(batch_names, num_count);
    EXPECT_EQ(dbMetaInfo.mega_batchs_.size(), 2);
    EXPECT_EQ(dbMetaInfo.mega_batchs_[0].TotalSize(), kPerBatchMinSize+300);
    EXPECT_EQ(dbMetaInfo.mega_batchs_[0].batch_names_.size(), 1);
    EXPECT_EQ(dbMetaInfo.mega_batchs_[0].data_cache_name_.empty(), false);
    EXPECT_EQ(dbMetaInfo.mega_batchs_[0].per_batch_size_.size(),1);
    EXPECT_EQ(dbMetaInfo.mega_batchs_[0].batch_count_.size(), 1);

    EXPECT_EQ(dbMetaInfo.mega_batchs_[1].TotalSize(), kPerBatchMinSize+200+kPerBatchMinSize-100);
    EXPECT_EQ(dbMetaInfo.mega_batchs_[1].batch_names_.size(), 2);
    EXPECT_EQ(dbMetaInfo.mega_batchs_[1].data_cache_name_.empty(), false);
    EXPECT_EQ(dbMetaInfo.mega_batchs_[1].per_batch_size_.size(),2);
    EXPECT_EQ(dbMetaInfo.mega_batchs_[1].batch_count_.size(), 2);
    EXPECT_EQ(dbMetaInfo.GetMergedBatch(0),1);
    EXPECT_EQ(dbMetaInfo.GetMergedBatch(1),0);
    EXPECT_EQ(dbMetaInfo.GetMergedBatch(2),1);
}

TEST_F(DbMetaInfoTest, SerializationAndDeserialization)
{
    // 创建一个 DbMetaInfo 对象
    DbMetaInfo info("key", "meta_path",key_columns_,label_columns_);
    info.SetBatch({"batch1", "batch2"}, {1, 2});

    // 序列化对象到文件
    const std::string filename = "db_meta_info.bin";
    info.Serialize(filename);

    // 反序列化文件到新的对象
    DbMetaInfo restoredInfo;
    cout<<"haha1"<<endl;
    restoredInfo.Deserialize(filename);
    cout<<"haha2"<<endl;

    // 执行断言，检查序列化前后对象的一致性
    EXPECT_EQ(info.key_, restoredInfo.key_);
    EXPECT_EQ(info.meta_path_, restoredInfo.meta_path_);
    EXPECT_EQ(info.total_batch_size_, restoredInfo.total_batch_size_);
    EXPECT_EQ(info.merged_batch_size_, restoredInfo.merged_batch_size_);
    EXPECT_EQ(info.mega_batchs_.size(), restoredInfo.mega_batchs_.size());
    for(std::size_t i=0;i<info.mega_batchs_.size();++i)
        EXPECT_EQ(info.mega_batchs_[i],restoredInfo.mega_batchs_[i]);
    EXPECT_EQ(info.batch_hash_.size(), restoredInfo.batch_hash_.size());
    for(std::size_t i=0;i<info.batch_hash_.size();++i)
        EXPECT_EQ(info.batch_hash_[i],restoredInfo.batch_hash_[i]);
}


TEST_F(DbMetaInfoTest, SerializeBatchHashAndDes)
{
    DbMetaInfo info;
    for(std::size_t i=0;i<21;++i)
    {
        info.batch_hash_.push_back(i*98);
        info.label_columns_.push_back(std::to_string(i*12));
    }
    info.merged_batch_size_  = 123;
    info.total_batch_size_ = 213;
    std::string str = DbBatchHashHelper::SerializeBatchHash(info);
    DbBatchHashHelper helper;
    helper.DeserializeBatchHash(str);
    EXPECT_EQ(info.batch_hash_.size(),helper.batch_hash_.size());
    for(std::size_t i=0;i<info.batch_hash_.size();++i)
    {
        EXPECT_EQ(info.batch_hash_[i],helper.batch_hash_[i]);
        EXPECT_EQ(info.GetMergedBatch(i),helper.GetMergedBatch(i));
        EXPECT_EQ(info.label_columns_[i],helper.label_columns_[i]);
    }
    EXPECT_EQ(info.total_batch_size_,helper.total_batch_size_);
    EXPECT_EQ(info.merged_batch_size_,helper.merged_batch_size_);
}
