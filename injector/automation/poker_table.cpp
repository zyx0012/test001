#include "poker_table.hpp"
#include "lobby.hpp"
#include "QtCore/qpointer.h"

namespace coinpoker {
namespace automation {

bool PokerTableHandler::matches(QWidget* w, QEvent* event) const {
	return (event->type() == QEvent::Polish || event->type() == QEvent::Close) &&
		QString(w->metaObject()->className()) == "CoinPokerTablePoker";
}

void PokerTableHandler::handle(QWidget* w, QEvent* event) {
	auto type = event->type();
	if (type == QEvent::Polish) {
		QPointer<QWidget> safeW = w;
		QTimer::singleShot(0, [safeW]() {
			Lobby::instance().addPokerTable(safeW);
			});
	}
	else if (type == QEvent::Close) {
		QPointer<QWidget> safeW = w;
		QTimer::singleShot(0, [safeW]() {
			Lobby::instance().removePokerTable(safeW);
			});
	}
}

}
}
