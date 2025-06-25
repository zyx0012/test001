#include "./login_page.hpp"
#include "QtWidgets/qwidget.h"
#include "QtWidgets/qlabel.h"
#include "QtCore/qcoreapplication.h"
#include "QtWidgets/qlineedit.h"
#include "QtWidgets/qpushbutton.h"
#include "QtCore/qstring.h"
#include "QtCore/qtimer.h"
#include "../utils/config.hpp"

namespace coinpoker {
namespace automation {

bool LoginPageHandler::matches(QWidget* w, QEvent* event) const {
	return event->type() == QEvent::ShowToParent &&
		w->objectName() == "loginPage";
}

void LoginPageHandler::handle(QWidget* w, QEvent* event) {
	auto loginUsername = w->findChild<QLineEdit*>("loginUserName");
	auto loginPassword = w->findChild<QLineEdit*>("loginPassword");
	auto loginButton = w->findChild<QPushButton*>("loginButton");
	loginUsername->setText(QString::fromStdString(utils::gConfig.username));
	loginPassword->setText(QString::fromStdString(utils::gConfig.password));
	QTimer::singleShot(500, loginButton, &QPushButton::click);
}

}
}
