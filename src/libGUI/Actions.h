#ifndef ACTIONS_H
#define ACTIONS_H

#include <QAction>
#include <QObject>
#include <QString>

class BookmarkAction : public QAction
{
    Q_OBJECT
public:
    BookmarkAction(const QString& name, QObject* parent);

public slots:
    void onTriggered();

Q_SIGNALS:
    void triggeredName(const QString& name);
    void test();
};

#endif // ACTIONS_H