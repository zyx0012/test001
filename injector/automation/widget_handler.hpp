#pragma once
#include "QtWidgets/qwidget.h"

namespace coinpoker {
namespace automation {
class WidgetHandler {
public:
	virtual ~WidgetHandler() = default;
	virtual bool matches(QWidget* w, QEvent* event) const = 0;
	virtual void handle(QWidget* w, QEvent* event) = 0;
};


class WidgetHandlerRegistry {
public:
	static WidgetHandlerRegistry& instance();

	void registerHandler(WidgetHandler* handler);
	void dispatch(QWidget* w, QEvent* event);
private:
	QList<WidgetHandler*> handlers_;
};

}



}
