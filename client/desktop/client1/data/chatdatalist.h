#ifndef CHATDATALIST_H
#define CHATDATALIST_H

#include "core/global.h"
#include "core/userdata.h"

class ChatThreadData;

// 记录为文件传输的信息在数据库上一次的更新信息
struct PersistState {
    qint64 lastSavedMs = 0; // 上一次更新的时间戳
    int lastSavedPercent = -1; // 上一次的percent
    TRANSFER_STATE lastStatus = TRANSFER_STATE::None; // 上一次的传输状态
};

struct FileTransferInfo
{
    TRANSFER_STATE transfer_status_ = TRANSFER_STATE::None;

    qint64 trans_size_ = 0;       // 已传输字节
    qint64 total_size_ = 0;       // 总字节（未知时可为 0）
    int progress_ = 0;            // 0~100（可缓存，便于 UI 直接显示）

    QString local_path_;          // 本地保存路径（完成后用于打开）

    QString mime_type_;           // （可选）便于图标/打开方式，比如 "application/pdf"
    QString error_;               // 失败原因（可选）
};

struct ChatDataItem
{
    ChatDataItem() = default;
    ChatDataItem(ChatRole role, QString name, QString icon, CHAT_MSG_TYPE msg_type,
                 QString content, MsgStatus status, QDateTime chat_time, qint64 diffSeconds, QString unique_id)
        : role_(role), name_(std::move(name)), icon_(std::move(icon)), msg_type_(msg_type), content_(std::move(content))
        , status_(status), chat_time_(chat_time), diffSeconds_(diffSeconds), unique_id_(std::move(unique_id))
    {
    }

    ChatRole role_;
    QString name_;
    QString icon_;
    CHAT_MSG_TYPE msg_type_;
    QString content_;        // TEXT: 文本; PIC/FILE: 文件url/标识
    MsgStatus status_;
    QDateTime chat_time_;
    qint64 diffSeconds_;
    QString unique_id_;
    // ✅ 新增：（TEXT 可保持默认 None）
    FileTransferInfo file_;
};

class ChatDataModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum ChatDataItemRoles
    {
        ChatUserRole = Qt::UserRole + 1, // 自己的还是对方的
        NameRole, // 发送消息人的名字
        IconRole, // 发送消息人的头像
        MsgTypeRole, // 发送消息的类型
        ContentRole, // 发送消息的内容
        StatusRole, // 发送消息的状态
        ChatTimeRole, // 发送消息的时间
        DiffSecondsRole, // 与上一条消息的时间间隔
        UniqueIdRole, // 每一条消息唯一的标识符
        FileTransSizeRole,// 文件已经传输的大小
        FileTotalSizeRole, // 文件总共的大小
        FileProgressRole, // 文件传输的进度（0-100）
        FileLocalPathRole, // 文件在本地的存储路径
        FileTransferStatusRole, // 文件传输的情况
        FileTransFailedRole, // 文件传输失败的原因
    };

    ChatDataModel(QObject *parent = nullptr);
    ~ChatDataModel();

    int GetRowCount() { return items_.size(); }

    void insertChatDataItem(std::shared_ptr<ChatDataBase> data, bool is_show_time);
    void insertChatDataItems(std::vector<std::shared_ptr<ChatDataBase>> datas);

    void appendChatDataItem(std::shared_ptr<ChatDataBase> data);
    void appendChatDataItems(QVector<std::shared_ptr<ChatDataBase>> datas);

    void resetChatDataItems(std::shared_ptr<ChatThreadData> thread_data);

    void updateItemStatus(QString unique_id, MsgStatus status); // 更新消息状态
    void updateMsgChatTime(QString unique_id, QDateTime chat_time); // 更新消息的时间
    void updateFileTransfer(const QString &unique_id, qint64 trans, qint64 total, TRANSFER_STATE st, QString error_reason); // 更新 Pic/File 文件的传输信息

    //void updateFileTransState(QString unique_name);

    //void onDownloadProgress(const QString& unique_id, qint64 trans, qint64 total, TRANSFER_STATE st, QString error_reason);

public slots:
    void slotUpdateUploadStatus(QString unique_name);
    void slotUpdateDownloadStatus(QString unique_name);
    void slotChangeTransStatus(QString unique_name);

protected:
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
    std::deque<ChatDataItem> items_;
    QHash<QString, int> row_by_id_; // unique_id : row
    QHash<QString, QString> uuid_by_content_; // unique_name : unique_id
    QHash<QString, PersistState> persist_; // unique_name : PersistState

signals:
    void signalAppendChatDataItem();
};

class ChatDataView : public QListView
{
    Q_OBJECT
public:
    ChatDataView(QWidget* parent = nullptr);
    ~ChatDataView();

    QModelIndex hoverIndex() const { return m_hoverIndex; }
    QPoint hoverPos() const { return m_hoverPos; }

    void setPressIndex(QModelIndex index = QModelIndex()) { m_pressIndex = index; }
    QModelIndex pressIndex() const { return m_pressIndex; }
    QPoint pressPos() const { return m_pressPos; }

    void SetCurrentThreadID(int thread_id) { cur_thread_id_ = thread_id; }
    void SetISsLoading(bool is_loading) { is_loading_ = is_loading; }
    void SetIsLoadingMoreMsg(int thread_id, bool is_load_more) { is_load_more_chatMsg_[thread_id] = is_load_more; }

protected:
    bool eventFilter(QObject* watched,QEvent* event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void updateHover(const QModelIndex &index, const QPoint &pos);

    void mousePressEvent(QMouseEvent *event) override;

private:
    QMap<int,bool> is_load_more_chatMsg_;
    int cur_thread_id_;
    bool is_loading_;

private:
    QModelIndex m_hoverIndex; // 鼠标悬浮时对应的QModelIndex
    QPoint m_hoverPos; // 鼠标悬浮的位置

    QModelIndex m_pressIndex; // 鼠标点击时对应的QModelIndex
    QPoint m_pressPos; // 鼠标点击的位置

    int m_overScrollAccumulated_ = 0;   // 累计向上滚动距离
    const int m_triggerThreshold_ = 120; // 触发阈值（可调）
};

class ChatDataDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    ChatDataDelegate(ChatDataView* view, QObject *parent = nullptr);
    ~ChatDataDelegate();

    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

protected:
    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    // 绘制头像
    void drawAvatar(QPainter *painter,const QRect &rect,const QModelIndex &index, const QString icon) const;

    // 1) emoji 友好的文本测量
    QSizeF measureTextLayout(const QString& text, const QFont& font, qreal maxWidth) const;

    // 2) 获取消息类型
    CHAT_MSG_TYPE msgTypeOf(const QModelIndex& index) const;

    // 3) 不同类型的 padding（气泡内边距）
    QMargins bubblePaddingForType(CHAT_MSG_TYPE type) const;

    // 4) 统一测量内容区（contentRect）尺寸：按类型分发
    QSize measureContentSize(const QModelIndex& index,
                             CHAT_MSG_TYPE type,
                             const QFont& font,
                             int maxContentWidth) const;

    // 5) 统一绘制内容区：按类型分发
    void drawContent(QPainter* p,
                     const QModelIndex& index,
                     CHAT_MSG_TYPE type,
                     const QFont& font,
                     const QRect& contentRect) const;

    // --- 分类型：TEXT ---
    QSize measureTextContent(const QModelIndex& index,
                             const QFont& font,
                             int maxContentWidth) const;
    void drawTextContent(QPainter* p,
                         const QModelIndex& index,
                         const QFont& font,
                         const QRect& contentRect) const;

    // --- 分类型：PIC ---
    QSize measurePicContent(const QModelIndex& index,
                            int maxContentWidth) const;
    void drawPicContent(QPainter* p,
                        const QModelIndex& index,
                        const QRect& contentRect) const;

    // --- 分类型：FILE（占位实现，后续你补字段再升级） ---
    QSize measureFileContent(const QModelIndex& index,
                             const QFont& font,
                             int maxContentWidth) const;
    void drawFileContent(QPainter* p,
                         const QModelIndex& index,
                         const QFont& font,
                         const QRect& contentRect) const;

    // 转换数文件大小的单位
    QString getFileSize(qint64 size) const;

    void PictureView(QString local_path) const;

    ChatDataView* m_view;

signals:
    void signalChangeTransStatus(QString) const;

};

class ChatDataList : public QWidget
{
    Q_OBJECT
public:
    ChatDataList(QWidget *parent = nullptr);
    ~ChatDataList();

    void appendChatItem(std::shared_ptr<ChatDataBase> data);
    void prependChatItem(std::shared_ptr<ChatDataBase> data);
    void prependChatItems(std::vector<std::shared_ptr<ChatDataBase>> datas);

    void resetAllItems(std::shared_ptr<ChatThreadData> cur_thread_data); // 清空所有的消息 // to do ... del

    void updateMsgStatus(QString unique_id, MsgStatus status);
    void updateMsgChatTime(QString unique_id,QDateTime chat_time);

    int GetCurThreadID() { return cur_thread_id_; }
    void SetCurThreadID(int thread_id) { cur_thread_id_ = thread_id; view_->SetCurrentThreadID(cur_thread_id_);}
    void setIsLoading(bool is_loading) { is_loading_ = is_loading; view_->SetISsLoading(is_loading); }

public slots:
    void slotNotifyIsLoadMoreMsg(int thread_id,bool is_more);

private:
    ChatDataModel* model_;
    ChatDataView* view_;
    ChatDataDelegate* delegate_;

    int cur_thread_id_;
    QMap<int,bool> is_load_more_chatMsg_;
    bool is_loading_;

private:
    QScrollArea *m_pScrollArea;
    bool isAppended;

};

#endif // CHATDATALIST_H
