#pragma once
#include <QApplication>
#include <QString>

namespace poker {

inline void applyTheme(QApplication& app) {
    app.setStyleSheet(R"(

/* ───────────────────────────────── Base ───────────────────────────────── */

QWidget {
    background-color: #0c0c11;
    color: #ddd8cc;
    font-family: "Segoe UI";
    font-size: 13px;
    border: none;
}

QMainWindow {
    background-color: #0c0c11;
}

/* ────────────────────────────── Frames ────────────────────────────────── */

QFrame#boardFrame {
    background: qradialgradient(cx:0.5, cy:0.5, radius:0.75,
        fx:0.5, fy:0.4,
        stop:0 #124528, stop:0.6 #0b3019, stop:1 #071c0f);
    border: 3px solid #1d5e35;
    border-radius: 120px;
}

QFrame#opponentSeat {
    background-color: transparent;
    border: none;
}

QFrame#humanSeat {
    background-color: transparent;
    border: none;
}

QFrame#divider {
    background-color: #1e2030;
    max-height: 1px;
    min-height: 1px;
}

QFrame#statusBar {
    background-color: #0f1a14;
    border: 1px solid #1a3020;
    border-radius: 6px;
}

/* ──────────────────────────── Action Buttons ───────────────────────────── */

QPushButton {
    background-color: #16161e;
    color: #ddd8cc;
    border: 1px solid #26263a;
    border-radius: 7px;
    padding: 8px 18px;
    font-size: 13px;
}
QPushButton:hover  { background-color: #20202e; border-color: #c4991f; }
QPushButton:pressed { background-color: #0c0c12; }
QPushButton:disabled { color: #383845; border-color: #18181f; background-color: #0f0f15; }

QPushButton#foldBtn {
    background-color: #2e0f0f;
    border-color: #6b1a1a;
    color: #e87070;
    font-weight: bold;
    font-size: 14px;
    min-height: 44px;
    border-radius: 8px;
}
QPushButton#foldBtn:hover    { background-color: #451515; border-color: #aa2424; }
QPushButton#foldBtn:pressed  { background-color: #1c0a0a; }
QPushButton#foldBtn:disabled { background-color: #160808; color: #4a2020; border-color: #200e0e; }

QPushButton#callBtn {
    background-color: #0b2e18;
    border-color: #196030;
    color: #70d898;
    font-weight: bold;
    font-size: 14px;
    min-height: 44px;
    border-radius: 8px;
}
QPushButton#callBtn:hover    { background-color: #124424; border-color: #25a050; }
QPushButton#callBtn:pressed  { background-color: #071a0e; }
QPushButton#callBtn:disabled { background-color: #080f0a; color: #1e4028; border-color: #0a1810; }

QPushButton#raiseBtn {
    background-color: #2a1e06;
    border-color: #7a5a10;
    color: #d4a830;
    font-weight: bold;
    font-size: 14px;
    min-height: 44px;
    border-radius: 8px;
}
QPushButton#raiseBtn:hover    { background-color: #3e2c08; border-color: #c4991f; }
QPushButton#raiseBtn:pressed  { background-color: #180f02; }
QPushButton#raiseBtn:disabled { background-color: #100c02; color: #3a3010; border-color: #1a1406; }

QPushButton#startBtn {
    background-color: #0b2e18;
    border: 2px solid #196030;
    color: #70d898;
    font-weight: bold;
    font-size: 15px;
    padding: 12px 28px;
    min-height: 50px;
    border-radius: 10px;
    letter-spacing: 1px;
}
QPushButton#startBtn:hover   { background-color: #124424; border-color: #25a050; }
QPushButton#startBtn:pressed { background-color: #071a0e; }

QPushButton#addBtn {
    background-color: #16161e;
    border-color: #26263a;
    color: #a0a0b8;
    font-size: 12px;
    padding: 6px 14px;
    border-radius: 6px;
}
QPushButton#addBtn:hover { border-color: #c4991f; color: #ddd8cc; }

QPushButton#removeBtn {
    background-color: #16161e;
    border-color: #26263a;
    color: #a0a0b8;
    font-size: 12px;
    padding: 6px 14px;
    border-radius: 6px;
}
QPushButton#removeBtn:hover { border-color: #6b1a1a; color: #e87070; }

/* ────────────────────────────── SpinBox ────────────────────────────────── */

QSpinBox {
    background-color: #111118;
    border: 1px solid #26263a;
    border-radius: 7px;
    color: #ddd8cc;
    padding: 6px 8px;
    font-size: 13px;
    min-width: 100px;
}
QSpinBox:hover  { border-color: #c4991f; }
QSpinBox:focus  { border-color: #c4991f; }

QSpinBox::up-button, QSpinBox::down-button {
    background-color: #1a1a24;
    border: none;
    width: 18px;
}
QSpinBox::up-button:hover, QSpinBox::down-button:hover {
    background-color: #26263a;
}

/* ────────────────────────────── ComboBox ───────────────────────────────── */

QComboBox {
    background-color: #111118;
    border: 1px solid #26263a;
    border-radius: 7px;
    color: #ddd8cc;
    padding: 6px 10px;
    font-size: 13px;
    min-width: 140px;
}
QComboBox:hover { border-color: #c4991f; }
QComboBox:focus { border-color: #c4991f; }

QComboBox::drop-down {
    border: none;
    width: 22px;
}

QComboBox QAbstractItemView {
    background-color: #111118;
    border: 1px solid #26263a;
    color: #ddd8cc;
    selection-background-color: #1d3a26;
    selection-color: #70d898;
    outline: none;
}

/* ─────────────────────────────── Labels ────────────────────────────────── */

QLabel#titleLabel {
    font-family: "Georgia";
    font-size: 36px;
    font-weight: bold;
    color: #c4991f;
}

QLabel#subtitleLabel {
    font-size: 11px;
    color: #60605e;
    letter-spacing: 3px;
}

QLabel#sectionLabel {
    font-size: 10px;
    color: #60605e;
    letter-spacing: 2px;
}

QLabel#potLabel {
    background-color: transparent;
    font-family: "Georgia";
    font-size: 20px;
    color: #c4991f;
    font-weight: bold;
}

QLabel#statusLabel {
    font-size: 13px;
    color: #90b8a0;
    letter-spacing: 1px;
    padding: 6px 14px;
}

QLabel#chipLabel {
    font-size: 12px;
    color: #c4991f;
    font-weight: bold;
}

QLabel#nameLabel {
    font-size: 13px;
    font-weight: bold;
    color: #ddd8cc;
}

QLabel#fileLabel {
    font-size: 11px;
    color: #50504e;
}

/* ─────────────────────────────── GroupBox ──────────────────────────────── */

QGroupBox {
    border: 1px solid #1e2030;
    border-radius: 10px;
    margin-top: 14px;
    padding-top: 6px;
    color: #50504e;
    font-size: 10px;
}
QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    left: 14px;
    top: -1px;
    letter-spacing: 2px;
}

/* ──────────────────────────── Scrollbar ────────────────────────────────── */

QScrollBar:vertical {
    background: #0c0c11;
    width: 7px;
    border-radius: 3px;
}
QScrollBar::handle:vertical {
    background: #26263a;
    border-radius: 3px;
    min-height: 24px;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }

/* ────────────────────────────── Badges ─────────────────────────────────── */

QLabel#dealerDot {
    background-color: #f5f0e8;
    border: 1px solid #c8c0b0;
    border-radius: 9px;
    color: #1a1a1a;
    font-size: 8px;
    font-weight: bold;
}
QLabel#sbDot, QLabel#bbDot {
    background-color: #3a6fd8;
    border: 1px solid #2a4fa8;
    border-radius: 9px;
    color: #f5f0e8;
    font-size: 8px;
    font-weight: bold;
}

/* ─────────────────────────── History log ───────────────────────────────── */

QListWidget#historyLog {
    background-color: #111118;
    border: 1px solid #1e2030;
    border-radius: 10px;
    padding: 8px;
}
QListWidget#historyLog::item {
    color: #a8a294;
    font-size: 12px;
    padding: 3px 2px;
    border: none;
}

/* ─────────────────────────── Outcome bubble ─────────────────────────────── */

QListWidget#outcomeLog {
    background-color: #111118;
    border: 1px solid #c4991f;
    border-radius: 10px;
    padding: 8px;
}
QListWidget#outcomeLog::item {
    color: #d4a830;
    font-size: 12px;
    font-weight: bold;
    padding: 3px 2px;
    border: none;
}

)");
}

} // namespace poker
