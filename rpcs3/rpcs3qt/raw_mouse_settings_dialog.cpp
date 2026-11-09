#include "stdafx.h"
#include "raw_mouse_settings_dialog.h"
#include "localized_emu.h"
#include "gui_application.h"
#include "Input/raw_mouse_config.h"
#include "Input/raw_mouse_handler.h"
#include "Emu/Io/gem_config.h"
#include "util/asm.hpp"

#include <QAbstractItemView>
#include <QGroupBox>
#include <QInputDialog>
#include <QLineEdit>
#include <QLabel>
#include <QSignalBlocker>
#include <QStringList>
#include <QMessageBox>
#include <QVBoxLayout>

LOG_CHANNEL(cfg_log, "CFG");

constexpr u32 output_button_count = 8;
constexpr u32 button_count = output_button_count * 2;

raw_mouse_settings_dialog::raw_mouse_settings_dialog(QWidget* parent, const std::string& title_id)
	: QDialog(parent), m_profile_title_id(title_id)
{
	setObjectName("raw_mouse_settings_dialog");
	setWindowTitle(tr("Configure Lightguns / Raw Mouse"));
	setAttribute(Qt::WA_DeleteOnClose);
	setAttribute(Qt::WA_StyledBackground);
	setModal(true);

	QVBoxLayout* v_layout = new QVBoxLayout(this);

	m_tab_widget = new QTabWidget();
	m_tab_widget->setUsesScrollButtons(false);

	m_button_box = new QDialogButtonBox(this);
	m_button_box->setStandardButtons(QDialogButtonBox::Apply | QDialogButtonBox::Cancel | QDialogButtonBox::Save | QDialogButtonBox::RestoreDefaults);

	connect(m_button_box, &QDialogButtonBox::clicked, this, [this](QAbstractButton* button)
		{
			if (button == m_button_box->button(QDialogButtonBox::Apply))
			{
				g_cfg_raw_mouse.save();
			}
			else if (button == m_button_box->button(QDialogButtonBox::Save))
			{
				g_cfg_raw_mouse.save();
				accept();
			}
			else if (button == m_button_box->button(QDialogButtonBox::RestoreDefaults))
			{
				if (QMessageBox::question(this, tr("Confirm Reset"), tr("Reset settings of all players?")) != QMessageBox::Yes)
					return;
				reset_config();
			}
			else if (button == m_button_box->button(QDialogButtonBox::Cancel))
			{
				// Restore config
				if (!g_cfg_raw_mouse.load(m_profile_title_id))
				{
					cfg_log.notice("Could not restore raw mouse config. Using defaults.");
				}
				reject();
			}
		});

	if (!g_cfg_raw_mouse.load(m_profile_title_id))
	{
		cfg_log.notice("Could not load raw mouse config. Using defaults.");
	}

	g_cfg_gem_mouse.load(); // Display the actual virtual-button to PS Move mapping.

	constexpr u32 max_devices = 16;

	g_raw_mouse_handler = std::make_unique<raw_mouse_handler>();
	g_raw_mouse_handler->set_is_for_gui(true);
	g_raw_mouse_handler->Init(std::max(max_devices, ::size32(g_cfg_raw_mouse.players)));
	g_raw_mouse_handler->set_mouse_press_callback([this](std::string_view device_name, s32 button_code, bool pressed)
		{
			mouse_press(device_name, button_code, pressed);
		});
	g_raw_mouse_handler->set_key_press_callback([this](std::string_view device_name, s32 scan_code, bool pressed)
		{
			key_press(device_name, scan_code, pressed);
		});

	m_buttons = new QButtonGroup(this);
	connect(m_buttons, &QButtonGroup::idClicked, this, &raw_mouse_settings_dialog::on_button_click);

	connect(&m_update_timer, &QTimer::timeout, this, &raw_mouse_settings_dialog::on_enumeration);

	connect(&m_remap_timer, &QTimer::timeout, this, [this]()
		{
			auto button = m_buttons->button(m_button_id);

			if (--m_seconds <= 0)
			{
				if (button)
				{
					if (const int button_id = m_buttons->id(button); button_id >= 0)
					{
						auto& config = ::at32(g_cfg_raw_mouse.players, m_tab_widget->currentIndex());
						const std::string name = raw_mouse_config::get_button_name(config->get_button_by_index(button_id % button_count).to_string());
						button->setText(name.empty() ? QStringLiteral("-") : QString::fromStdString(name));
					}
				}
				reactivate_buttons();
				return;
			}
			if (button)
			{
				button->setText(tr("[ Waiting %1 ]").arg(m_seconds));
			}
		});

	connect(&m_mouse_release_timer, &QTimer::timeout, this, [this]()
		{
			m_mouse_release_timer.stop();
			m_disable_mouse_release_event = false;
		});

	add_tabs(m_tab_widget);

	m_profile_button = new QPushButton(m_profile_title_id.empty() ? tr("Profile: Global defaults") : tr("Profile: %1").arg(QString::fromStdString(m_profile_title_id)), this);
	connect(m_profile_button, &QPushButton::clicked, this, [this]()
		{
			bool accepted = false;
			const QString id = QInputDialog::getText(this, tr("Lightgun game profile"),
				tr("Game title ID (for example BCUS98222). Leave blank for global defaults."),
				QLineEdit::Normal, QString::fromStdString(m_profile_title_id), &accepted)
		                           .trimmed()
		                           .toUpper();
			if (!accepted || id.toStdString() == m_profile_title_id)
				return;
			if (!id.isEmpty() && (id.size() != 9 || !std::all_of(id.begin(), id.end(), [](QChar c)
														{
															return (c >= QLatin1Char('A') && c <= QLatin1Char('Z')) || (c >= QLatin1Char('0') && c <= QLatin1Char('9'));
														})))
			{
				QMessageBox::warning(this, tr("Invalid title ID"), tr("Enter a nine-character title ID using letters and numbers."));
				return;
			}
			const auto choice = QMessageBox::question(this, tr("Change profile"), tr("Save the current profile before switching?"),
				QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Cancel);
			if (choice == QMessageBox::Cancel)
				return;
			if (choice == QMessageBox::Save)
				g_cfg_raw_mouse.save();
			m_profile_title_id = id.toStdString();
			g_cfg_raw_mouse.load(m_profile_title_id);
			m_profile_button->setText(id.isEmpty() ? tr("Profile: Global defaults") : tr("Profile: %1").arg(id));
			refresh_config_widgets();
		});
	v_layout->addWidget(m_profile_button);
	QLabel* mapping_note = new QLabel(tr("Each PS Move action accepts either binding. Optional game labels are notes; verify them in the game.\nPS Move action routing is read from Configuration > PS Move > Mouse."), this);
	mapping_note->setWordWrap(true);
	v_layout->addWidget(mapping_note);
	v_layout->addWidget(m_tab_widget);
	v_layout->addWidget(m_button_box);
	setLayout(v_layout);

	m_palette = m_push_buttons[0][0]->palette(); // save normal palette

	connect(m_tab_widget, &QTabWidget::currentChanged, this, [this](int index)
		{
			handle_device_change(get_current_device_name(index));
		});

	on_enumeration();

	handle_device_change(get_current_device_name(0));

	m_update_timer.start(1000ms);
}

raw_mouse_settings_dialog::~raw_mouse_settings_dialog()
{
	g_raw_mouse_handler.reset();
}

void raw_mouse_settings_dialog::update_combo_box(usz player)
{
	auto& config = ::at32(g_cfg_raw_mouse.players, player);
	const std::string device_name = config->device.to_string();
	const QString current_device = QString::fromStdString(device_name);
	bool found_device = false;

	QComboBox* combo = ::at32(m_device_combos, player);

	combo->blockSignals(true);
	combo->clear();
	combo->addItem(tr("Disabled"), QString());

	{
		std::lock_guard lock(g_raw_mouse_handler->m_raw_mutex);

		const auto& mice = g_raw_mouse_handler->get_mice();

		for (const auto& [handle, mouse] : mice)
		{
			const QString name = QString::fromStdString(mouse.device_name());
			const QString& pretty_name = name; // Same ugly device path for now
			combo->addItem(pretty_name, name);

			if (current_device == name)
			{
				found_device = true;
			}
		}
	}

	// Keep configured device in list even if it is not connected
	if (!found_device && !current_device.isEmpty())
	{
		const QString& pretty_name = current_device; // Same ugly device path for now
		combo->addItem(pretty_name, current_device);
	}

	// Select index
	combo->setCurrentIndex(std::max(0, combo->findData(current_device)));
	combo->blockSignals(false);

	if (static_cast<int>(player) == m_tab_widget->currentIndex())
	{
		handle_device_change(device_name);
	}
}

void raw_mouse_settings_dialog::add_tabs(QTabWidget* tabs)
{
	ensure(!!tabs);

	constexpr u32 max_items_per_column = 6;
	int rows = output_button_count;

	for (u32 cols = 1; utils::aligned_div(output_button_count, cols) > max_items_per_column;)
	{
		rows = utils::aligned_div(output_button_count, ++cols);
	}

	constexpr usz players = g_cfg_raw_mouse.players.size();

	m_push_buttons.resize(players);
	m_action_labels.resize(players);

	ensure(g_raw_mouse_handler);

	const auto insert_button = [this](int id, QPushButton* button)
	{
		m_buttons->addButton(button, id);
		button->installEventFilter(this);
	};

	for (usz player = 0; player < players; player++)
	{
		QWidget* widget = new QWidget(this);
		QGridLayout* grid_layout = new QGridLayout(this);

		auto& config = ::at32(g_cfg_raw_mouse.players, player);

		QHBoxLayout* h_layout = new QHBoxLayout(this);
		QGroupBox* gb = new QGroupBox(tr("Device"), this);
		QComboBox* combo = new QComboBox(this);
		m_device_combos.push_back(combo);

		update_combo_box(player);

		connect(combo, &QComboBox::currentIndexChanged, this, [this, player, combo](int index)
			{
				if (index < 0 || !combo)
					return;

				const QVariant data = combo->itemData(index);
				if (!data.isValid() || !data.canConvert<QString>())
					return;

				const std::string device_name = data.toString().toStdString();

				auto& config = ::at32(g_cfg_raw_mouse.players, player)->device;
				config.from_string(device_name);

				handle_device_change(device_name);
			});

		h_layout->addWidget(combo);
		gb->setLayout(h_layout);
		grid_layout->addWidget(gb, 0, 0, 1, 2);

		const int first_row = grid_layout->rowCount();

		for (int i = 0, row = first_row, col = 0; i < static_cast<int>(output_button_count); i++, row++)
		{
			const QString translated_cell_button = action_title(player, i);

			QHBoxLayout* h_layout = new QHBoxLayout(this);
			QGroupBox* gb = new QGroupBox(translated_cell_button, this);
			if (row >= rows + first_row)
			{
				row = first_row;
				col++;
			}
			QVBoxLayout* action_layout = new QVBoxLayout;
			QLineEdit* label = new QLineEdit(QString::fromStdString(config->get_action_label_by_index(i).to_string()));
			label->setPlaceholderText(tr("Game action label (optional)"));
			label->setMaxLength(64);
			label->setToolTip(tr("A descriptive note such as Reload. This does not change the PS Move action."));
			m_action_labels[player][i] = label;
			connect(label, &QLineEdit::textEdited, this, [player, i](const QString& text)
				{
					::at32(g_cfg_raw_mouse.players, player)->get_action_label_by_index(i).from_string(text.toStdString());
				});
			action_layout->addWidget(label);
			for (int slot = 0; slot < 2; slot++)
			{
				const int binding_index = i + slot * output_button_count;
				QPushButton* pb = new QPushButton;
				pb->setToolTip(slot == 0 ? tr("Primary input (mouse or keyboard)") : tr("Alternative input: either binding activates this button"));
				insert_button(static_cast<int>(player * button_count + binding_index), pb);
				const std::string saved_btn = raw_mouse_config::get_button_name(config->get_button_by_index(binding_index).to_string());
				pb->setText(saved_btn.empty() ? QStringLiteral("-") : QString::fromStdString(saved_btn));
				m_push_buttons[player][binding_index] = pb;
				QVBoxLayout* binding_layout = new QVBoxLayout;
				binding_layout->addWidget(new QLabel(slot == 0 ? tr("Primary") : tr("Alternative")));
				binding_layout->addWidget(pb);
				h_layout->addLayout(binding_layout);
			}
			action_layout->addLayout(h_layout);
			gb->setLayout(action_layout);
			grid_layout->addWidget(gb, row, col);
		}

		h_layout = new QHBoxLayout(this);
		gb = new QGroupBox(tr("Mouse Acceleration"), this);
		QDoubleSpinBox* mouse_acceleration_spin_box = new QDoubleSpinBox(this);
		m_accel_spin_boxes.push_back(mouse_acceleration_spin_box);
		mouse_acceleration_spin_box->setRange(0.1, 10.0);
		mouse_acceleration_spin_box->setValue(config->mouse_acceleration.get() / 100.0);
		connect(mouse_acceleration_spin_box, &QDoubleSpinBox::valueChanged, this, [player](double value)
			{
				auto& config = ::at32(g_cfg_raw_mouse.players, player)->mouse_acceleration;
				config.set(std::clamp(value * 100.0, config.min, config.max));
			});

		h_layout->addWidget(mouse_acceleration_spin_box);
		gb->setLayout(h_layout);
		grid_layout->addWidget(gb, grid_layout->rowCount(), 0);

		widget->setLayout(grid_layout);
		tabs->addTab(widget, tr("Player %0").arg(player + 1));
	}
}

QString raw_mouse_settings_dialog::action_title(usz player, int button_index) const
{
	const auto virtual_button = static_cast<pad_button>(static_cast<int>(pad_button::mouse_button_1) + button_index);
	const std::array<std::pair<gem_btn, QString>, 8> actions{{{gem_btn::t, tr("T / Trigger")}, {gem_btn::move, tr("Move")},
		{gem_btn::square, tr("Square")}, {gem_btn::circle, tr("Circle")},
		{gem_btn::cross, tr("Cross")}, {gem_btn::start, tr("Start")},
		{gem_btn::select, tr("Select")}, {gem_btn::triangle, tr("Triangle")}}};
	QStringList names;
	for (const auto& [action, name] : actions)
	{
		if (::at32(g_cfg_gem_mouse.players, player)->get_pad_button(action) == virtual_button)
			names << name;
	}
	const QString button_name = localized_emu::translated_mouse_button(get_mouse_button_code(button_index));
	return names.empty() ? tr("Unassigned (%1)").arg(button_name) : tr("%1 (%2)").arg(names.join(QStringLiteral(" + ")), button_name);
}

void raw_mouse_settings_dialog::on_enumeration()
{
	if (!g_raw_mouse_handler)
	{
		return;
	}

	const int player = m_tab_widget->currentIndex();
	QComboBox* combo = ::at32(m_device_combos, player);

	// Don't enumerate while the current dropdown is open
	if (combo && combo->view()->isVisible())
	{
		return;
	}

	g_raw_mouse_handler->update_devices();

	// Update all dropdowns
	for (usz player = 0; player < g_cfg_raw_mouse.players.size(); player++)
	{
		update_combo_box(player);
	}
}

void raw_mouse_settings_dialog::reset_config()
{
	g_cfg_raw_mouse.from_default();
	refresh_config_widgets();
}

void raw_mouse_settings_dialog::refresh_config_widgets()
{
	for (usz player = 0; player < m_push_buttons.size(); player++)
	{
		auto& config = ::at32(g_cfg_raw_mouse.players, player);

		for (int i = 0; i < static_cast<int>(output_button_count); i++)
		{
			QLineEdit* label = m_action_labels[player][i];
			const QSignalBlocker blocker(label);
			label->setText(QString::fromStdString(config->get_action_label_by_index(i).to_string()));
		}
		update_combo_box(player);

		for (auto& [binding_index, pb] : m_push_buttons[player])
		{
			if (!pb)
				continue;

			const std::string text = raw_mouse_config::get_button_name(config->get_button_by_index(binding_index).to_string());
			pb->setText(text.empty() ? QStringLiteral("-") : QString::fromStdString(text));
		}

		if (QComboBox* combo = ::at32(m_device_combos, player))
		{
			combo->setCurrentIndex(combo->findData(QString::fromStdString(config->device.to_string())));
		}

		if (QDoubleSpinBox* sb = ::at32(m_accel_spin_boxes, player))
		{
			sb->setValue(config->mouse_acceleration.get() / 100.0);
		}
	}
}

void raw_mouse_settings_dialog::mouse_press(std::string_view device_name, s32 button_code, bool pressed)
{
	if (m_button_id < 0 || pressed) // Let's only react to mouse releases
	{
		return;
	}

	const int player = m_tab_widget->currentIndex();
	const std::string current_device_name = get_current_device_name(player);

	if (device_name != current_device_name)
	{
		return;
	}

	const std::string button_name = raw_mouse_config::get_button_name(button_code);

	auto& config = ::at32(g_cfg_raw_mouse.players, player);
	config->get_button_by_index(m_button_id % button_count).from_string(button_name);

	if (auto button = m_buttons->button(m_button_id))
	{
		button->setText(QString::fromStdString(button_name));
	}

	reactivate_buttons();
}

void raw_mouse_settings_dialog::key_press(std::string_view device_name, s32 scan_code, bool pressed)
{
	if (m_button_id < 0 || !pressed) // Let's only react to key presses
	{
		return;
	}

	const int player = m_tab_widget->currentIndex();
	const std::string current_device_name = get_current_device_name(player);

	if (device_name != current_device_name)
	{
		return;
	}

	auto& config = ::at32(g_cfg_raw_mouse.players, player);
	config->get_button_by_index(m_button_id % button_count).from_string(fmt::format("%s%d", raw_mouse_config::key_prefix, scan_code));

	if (auto button = m_buttons->button(m_button_id))
	{
		button->setText(QString::fromStdString(raw_mouse_config::get_key_name(scan_code)));
	}

	reactivate_buttons();
}

void raw_mouse_settings_dialog::handle_device_change(std::string_view device_name)
{
	if (is_device_active(device_name))
	{
		reactivate_buttons();
	}
	else
	{
		for (auto but : m_buttons->buttons())
		{
			but->setEnabled(false);
		}
	}
}

bool raw_mouse_settings_dialog::is_device_active(std::string_view device_name)
{
	if (!g_raw_mouse_handler || device_name.empty())
	{
		return false;
	}

	std::lock_guard lock(g_raw_mouse_handler->m_raw_mutex);

	const auto& mice = g_raw_mouse_handler->get_mice();

	return std::any_of(mice.cbegin(), mice.cend(), [&device_name](const auto& entry)
		{
			return entry.second.device_name() == device_name;
		});
}

std::string raw_mouse_settings_dialog::get_current_device_name(int player)
{
	if (player < 0)
		return {};

	const QVariant data = ::at32(m_device_combos, player)->currentData();

	if (!data.canConvert<QString>())
		return {};

	return data.toString().toStdString();
}

bool raw_mouse_settings_dialog::eventFilter(QObject* object, QEvent* event)
{
	switch (event->type())
	{
	case QEvent::MouseButtonRelease:
	{
		// On right click clear binding if we are not remapping pad button
		if (m_button_id < 0 && !m_disable_mouse_release_event)
		{
			QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);
			if (const auto button = qobject_cast<QPushButton*>(object); button && button->isEnabled() && mouse_event->button() == Qt::RightButton)
			{
				if (const int button_id = m_buttons->id(button); button_id >= 0)
				{
					button->setText(QStringLiteral("-"));
					auto& config = ::at32(g_cfg_raw_mouse.players, m_tab_widget->currentIndex());
					config->get_button_by_index(button_id % button_count).from_string("");
					return true;
				}
			}
		}

		// Disabled buttons should not absorb mouseclicks
		event->ignore();
		break;
	}
	default:
	{
		break;
	}
	}

	return QDialog::eventFilter(object, event);
}

void raw_mouse_settings_dialog::on_button_click(int id)
{
	if (id < 0)
	{
		return;
	}

	m_update_timer.stop();

	for (auto sb : m_accel_spin_boxes)
	{
		sb->setEnabled(false);
		sb->setFocusPolicy(Qt::ClickFocus);
	}

	for (auto cb : m_device_combos)
	{
		cb->setEnabled(false);
		cb->setFocusPolicy(Qt::ClickFocus);
	}

	for (auto but : m_buttons->buttons())
	{
		but->setEnabled(false);
		but->setFocusPolicy(Qt::ClickFocus);
	}

	m_profile_button->setEnabled(false);
	m_button_box->setEnabled(false);
	for (auto but : m_button_box->buttons())
	{
		but->setFocusPolicy(Qt::ClickFocus);
	}

	m_button_id = id;
	if (auto button = m_buttons->button(m_button_id))
	{
		button->setText(tr("[ Waiting %1 ]").arg(MAX_SECONDS));
		button->setPalette(QPalette(Qt::blue));
		button->grabMouse();
	}

	m_tab_widget->setEnabled(false);

	m_remap_timer.start(1000);

	// We need to disable the mouse release event filter or we will clear the button if the raw mouse does a right click
	m_mouse_release_timer.stop();
	m_disable_mouse_release_event = true;
}

void raw_mouse_settings_dialog::reactivate_buttons()
{
	m_remap_timer.stop();
	m_seconds = MAX_SECONDS;

	if (m_button_id >= 0)
	{
		if (auto button = m_buttons->button(m_button_id))
		{
			button->setPalette(m_palette);
			button->releaseMouse();
		}

		m_button_id = -1;
	}

	// Enable all buttons
	m_profile_button->setEnabled(true);
	m_button_box->setEnabled(true);

	for (auto but : m_button_box->buttons())
	{
		but->setFocusPolicy(Qt::StrongFocus);
	}

	for (auto sb : m_accel_spin_boxes)
	{
		sb->setEnabled(true);
		sb->setFocusPolicy(Qt::StrongFocus);
	}

	for (auto cb : m_device_combos)
	{
		cb->setEnabled(true);
		cb->setFocusPolicy(Qt::StrongFocus);
	}

	for (auto but : m_buttons->buttons())
	{
		but->setEnabled(true);
		but->setFocusPolicy(Qt::StrongFocus);
	}

	m_tab_widget->setEnabled(true);

	m_mouse_release_timer.start(100ms);
	m_update_timer.start(1000ms);
}
