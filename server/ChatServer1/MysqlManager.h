#ifndef MYSQLMANAGER_H
#define MYSQLMANAGER_H

// init �������ӵ����ݿ����ǣ�jerrychat

// �����û����ظ������ʱ����Ҫ���ݿ��������һ�£���Ҫȥ�жϷ��صĴ���ֵ

// ��mysqlc������Ϊmysqlc++��

#include "global.h"
#include "MysqlDao.h"

class User;
struct ApplyInfo;

class MysqlManager : public SingleTon<MysqlManager>
{
	friend class SingleTon<MysqlManager>;
	
public:
	~MysqlManager() {}
	// ע�����û�
	int registerUser(const std::string& name, const std::string& email, const std::string& password);
	// ��ȡ��Ӧ�û��ĺ�������
	int getUserFriendApply(int uid, std::vector<std::shared_ptr<ApplyInfo>>& applyList, bool forceMaster = false);	// ��ȡĳ�û��ĺ����б�
	int getUserFriendList(int uid, std::vector<std::shared_ptr<UserInfo>>& friendList, bool forceMaster = false);	// �������������ӵ����ݿ�
	int addFriendApply(int fromuid, int touid,int& current_id, std::string& apply_time);
	// ���Ӻ��ѹ�ϵ
	int addFriendRelation(int fromuid, int touid, int& thread_id, int& thread_id2, int& friend_id1, int& friend_id2);
	// ����uid��ȡ�û���Ϣ
	std::shared_ptr<UserInfo> getUserByUid(int uid, bool forceMaster = false);	// ����name��ȡ�û���Ϣ
	std::shared_ptr<UserInfo> getUserByName(std::string name, bool forceMaster = false);	// ���ú��������״̬
	int setFriendApplyStatus(int fromuid, int touid, int status);
	// ��ȡ�û��������б�
	int GetUserThreadInfos(int uid, int last_thread_id, int page_size, std::vector<std::shared_ptr<ChatThreadInfo>>& infos, bool& load_more, int& max_thread_id, bool forceMaster = false);	// ����˽���߳�
	int createPrivateThread(int user1_id, int user2_id, int& thread_id);
	// ����������Ϣ
	int AddChatMsg(std::vector<std::shared_ptr<ChatMessage>>& chat_datas);
	// ���غ����б�
	int getUserFriendListByLastId(int uid, int last_friend_id, std::map<int, std::shared_ptr<UserInfo>>& friend_list, bool forceMaster = false);	// ���غ��������б�
	int getUserFriendApplyByLastId(int uid, int last_friend_id, int page_size, std::vector<std::shared_ptr<ApplyInfo>>& applyList, bool& load_more, int& max_friend_apply_id, bool forceMaster = false);	// �޸�ChatMessage��Ϣ��״̬
	int updateChatMsgStatus(int message_id, MsgStatus status);
	// ����������Ϣ
	int loadChatMessage(int thread_id, int& min_message_id, int& max_message_id, int page_size, bool& is_more, std::vector<ChatMessage>& msgs, bool forceMaster = false);
private:
	MysqlManager() {}
	MysqlDao dao_;
};

#endif