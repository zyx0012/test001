#include "QtWidgets/qwidget.h"
#include "QtWidgets/qlabel.h"
#include "QtWidgets/qpushbutton.h"
#include "./widget_handler.hpp"
#include "QtCore/qcoreapplication.h"
#include "QtCore/qtimer.h"
#include "./messagebox.hpp"


namespace coinpoker {
namespace automation {


class ButtonClicker : public QObject {
    QPushButton* button;
public:
    ButtonClicker(QPushButton* b) : button(b) {
        button->installEventFilter(this);
    }

    bool eventFilter(QObject* obj, QEvent* event) override {
        if (event->type() == QEvent::Show) {
            if (button->isVisible() && button->isEnabled()) {
                QTimer::singleShot(0, [this] {
                    button->click();
                    deleteLater();
                });
            }
        }
        return QObject::eventFilter(obj, event);
    }
};

class PopupWatcher : public QObject {
public:
	bool eventFilter(QObject* obj, QEvent* event) override {
		if (event->type() != QEvent::ChildAdded) return false;
		auto childEvent = static_cast<QChildEvent*>(event);
		QObject* child = childEvent->child();
		auto objName = child->objectName().toStdString();
		if (objName != "ActionButton") return false;
		QPushButton* button = qobject_cast<QPushButton*>(child);
		if (button->text() == "Yes") {
			new ButtonClicker(button);
		}
		return false;
	}
};

bool MessageBoxHandler::matches(QWidget* w, QEvent* event) const {
	return event->type() == QEvent::ShowToParent &&
		w->objectName() == "messageBoxContent" && QString(w->metaObject()->className()) == "QWidget";
}

void MessageBoxHandler::handle(QWidget* w, QEvent* event) {
	auto* content = w->findChild<QWidget*>("content");
	if (!content) return;
	auto body = content->findChild<QWidget*>("body");
	auto buttons = content->findChild<QWidget*>("buttons");
	if (!body || !buttons) return;
	auto message = body->findChild<QLabel*>("message");
	if (!message) return;
	auto popupMessage = message->text().toStdString();
	if (popupMessage.find("You are seated on this table") != std::string::npos) {
		buttons->installEventFilter(new PopupWatcher());
	}
}
};
}
