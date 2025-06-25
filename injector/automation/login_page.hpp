#pragma once
#include "QtWidgets/qwidget.h"
#include "./widget_handler.hpp"


namespace coinpoker {
namespace automation {

class LoginPageHandler : public WidgetHandler {
public:
	bool matches(QWidget* w, QEvent* event) const override;
	void handle(QWidget* w, QEvent*) override;
};
}
}
