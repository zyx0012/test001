#include "./automation.hpp"
#include "QtCore/qcoreapplication.h"
#include "QtCore/qtimer.h"
#include <QtCore/QMetaMethod>
#include "QtWidgets/qwidget.h"
#include "QtWidgets/qlineedit.h"
#include "./widget_handler.hpp"
#include "QtWidgets/qpushbutton.h"
#include "QtWidgets/qtableview.h"
#include "QtWidgets/qapplication.h"
#include "QtWidgets/qlabel.h"
#include "QtGui/qpixmap.h"
#include <chrono>
#include <thread>
#include "lobby.hpp"
#include "./login_page.hpp"
#include "./messagebox.hpp"
#include "./widget_handler.hpp"
#include "./poker_table.hpp"

namespace coinpoker {
namespace automation {

std::shared_ptr<IInternalFunctions> Automation::internalFunctions_ = {};

class GuiEventWatcher : public QObject {

public:
	bool eventFilter(QObject* obj, QEvent* event) override {
		auto* w = qobject_cast<QWidget*>(obj);
		if (!w) return false;
		WidgetHandlerRegistry::instance().dispatch(w, event);
		if (event->type() == QEvent::FocusIn && QString(w->metaObject()->className()) == "CoinPokerTablePoker") {
			return true;
		}
		return false;
	}
};

int Automation::onGuiStart() {
	auto app = QCoreApplication::instance();
	WidgetHandlerRegistry::instance().registerHandler(new MessageBoxHandler());
	WidgetHandlerRegistry::instance().registerHandler(new LobbyListHandler());
	WidgetHandlerRegistry::instance().registerHandler(new LoginPageHandler());
	WidgetHandlerRegistry::instance().registerHandler(new PokerTableHandler());
	app->installEventFilter(new GuiEventWatcher());
	return internalFunctions_->applcationExec();
}

void Automation::setInternalFunctions(std::shared_ptr<IInternalFunctions> qtFunctions) {
	internalFunctions_ = std::move(qtFunctions);
}

}
}