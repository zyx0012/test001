#pragma once
#include "QtWidgets/qwidget.h"
#include "QtWidgets/qlabel.h"
#include "QtWidgets/qpushbutton.h"
#include "QtCore/qcoreapplication.h"
#include "QtCore/qtimer.h"
#include "./widget_handler.hpp"

namespace coinpoker {
namespace automation {

class PokerTableHandler : public WidgetHandler {
public:
	bool matches(QWidget* w, QEvent* event) const override;
	void handle(QWidget* w, QEvent*) override;
};

class PokerTableWrapper {
public:
	PokerTableWrapper() {};

	PokerTableWrapper(QWidget* widget) : widget_(widget) {}

	void close() const {
		widget_->close();
	}

	uint32_t getTableId() {
		return *(uint32_t*)((char*)(widget_)+0x340);
	}

	QWidget* getWidget() {
		return widget_;
	}

private:
	QWidget* widget_;
};
};
}
