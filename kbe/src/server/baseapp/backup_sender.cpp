#include "baseapp.hpp"
#include "backup_sender.hpp"
#include "server/serverconfig.hpp"

namespace KBEngine{	
float backupPeriod = 0.0;

//-------------------------------------------------------------------------------------
BackupSender::BackupSender():
backupEntityIDs_(),
backupRemainder_(0.f)
{
}

//-------------------------------------------------------------------------------------
BackupSender::~BackupSender()
{
	backupEntityIDs_.clear();
}

//-------------------------------------------------------------------------------------
void BackupSender::tick()
{
	int32 periodInTicks = secondsToTicks(ServerConfig::getSingleton().getBaseApp().backupPeriod, 0);
	if (periodInTicks == 0)
		return;

	// 这里对备份的entity做一下分批操作
	// 大概算法是配置上填写的备份周期换算成tick数量， 每个tick备份一部分entity
	float numToBackUpFloat = float(Baseapp::getSingleton().pEntities()->size()) / periodInTicks + backupRemainder_;

	// 本次备份的数量
	int numToBackUp = int(numToBackUpFloat);

	// 计算出精度导致的损失数量
	backupRemainder_ = numToBackUpFloat - numToBackUp;

	// 如果备份表中没有内容了则重新产生一份新的
	if (backupEntityIDs_.empty())
	{
		this->createBackupTable();
	}

	Mercury::Bundle bundle;
	while((numToBackUp > 0) && !backupEntityIDs_.empty())
	{
		Base * pBase = Baseapp::getSingleton().findEntity(backupEntityIDs_.back());
		backupEntityIDs_.pop_back();
		
		MemoryStream* s = MemoryStream::ObjPool().createObject();
		if (pBase && backup(*pBase, *s))
		{
			--numToBackUp;
			bundle.append(*s);
		}
		MemoryStream::ObjPool().reclaimObject(s);
	}
}

//-------------------------------------------------------------------------------------
bool BackupSender::backup(Base& base, MemoryStream& s)
{
	// 这里开始将需要备份的数据写入流
	base.writeBackupData(&s);
	return true;
}

//-------------------------------------------------------------------------------------
void BackupSender::createBackupTable()
{
	backupEntityIDs_.clear();

	Entities<Base>::ENTITYS_MAP::const_iterator iter = Baseapp::getSingleton().pEntities()->getEntities().begin();

	for(; iter != Baseapp::getSingleton().pEntities()->getEntities().end(); iter++)
	{
		backupEntityIDs_.push_back(iter->first);
	}

	// 随机一下序列
	std::random_shuffle(backupEntityIDs_.begin(), backupEntityIDs_.end());
}

}
