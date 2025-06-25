#include "./widget_handler.hpp"


namespace coinpoker {
namespace automation {

WidgetHandlerRegistry& WidgetHandlerRegistry::instance() {
	static WidgetHandlerRegistry inst;
	return inst;
}

void WidgetHandlerRegistry::registerHandler(WidgetHandler* handler) {
	handlers_.append(handler);
}

void WidgetHandlerRegistry::dispatch(QWidget* w, QEvent* event) {
	for (auto* handler : handlers_) {
		if (handler->matches(w, event)) {
			handler->handle(w, event);
			break;
		}
	}
}
};
}
