#include "painterdemo.h"
#include <QColor>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QTimer>

PainterDemo::PainterDemo(QWidget *parent, Qt::WFlags flags)
    : QMainWindow(parent, flags) {
  ui.setupUi(this);
  init();
  setMinimumSize(1300, 600);
}

PainterDemo::~PainterDemo() {}

void PainterDemo::init() {
  drawing_ = false;
  start_pos_ = QPoint(0, 0);
  end_pos_ = QPoint(0, 0);
  canvas_img_ = QImage(640, 480, QImage::Format_ARGB32_Premultiplied);
  canvas_img_.fill(qRgba(0, 66, 66, 66));
  canvas_cache_img_ = canvas_img_;
  draw_type_ = DRAW_TYPE::DT_PEN;
  draw_cmd_list_.clear();

  remote_canvas_label_ = new QLabel(this);
  remote_canvas_label_->setFixedSize(640, 480);
  remote_canvas_label_->move(650, 0);
  remote_canvas_img_ = canvas_img_;
  remote_canvas_cache_img_ = remote_canvas_img_;
  remote_canvas_label_->setPixmap(QPixmap::fromImage(remote_canvas_img_));
  remote_canvas_label_->show();

  undo_btn_ = new QPushButton("undo", this);
  connect(undo_btn_, SIGNAL(clicked(bool)), this, SLOT(on_undo_clicked(bool)));
  undo_btn_->move(0, 480);
  undo_btn_->setEnabled(false);

  re_undo_btn_ = new QPushButton("re-undo", this);
  connect(re_undo_btn_, SIGNAL(clicked(bool)), this,
          SLOT(on_re_undo_clicked(bool)));
  re_undo_btn_->move(undo_btn_->pos().x() + undo_btn_->width(), 480);
  re_undo_btn_->setEnabled(false);

  ellipse_btn_ = new QPushButton("Ellipse", this);
  connect(ellipse_btn_, SIGNAL(clicked(bool)), this,
          SLOT(on_ellipse_clicked(bool)));
  ellipse_btn_->move(0, 480 + undo_btn_->height());
  ellipse_btn_->setCheckable(true);
}

void PainterDemo::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    drawing_ = true;
    start_pos_ = event->pos();
    new_draw_ = true;
  }
}

void PainterDemo::mouseMoveEvent(QMouseEvent *event) {
  if (event->buttons() & Qt::LeftButton) {
    if (new_draw_) {
      start_new_stroke();
      new_draw_ = false;
    }
    end_pos_ = event->pos();
    canvas_cache_img_ = canvas_img_;
    if (draw_type_ == DRAW_TYPE::DT_PEN) {
      draw(canvas_img_);
    } else {
      draw(canvas_cache_img_);
    }
  }
}

void PainterDemo::mouseReleaseEvent(QMouseEvent *event) {
  new_draw_ = false;
  drawing_ = false;
  if (draw_type_ != DRAW_TYPE::DT_PEN) {
    if (draw_type_ == DRAW_TYPE::DT_ELLIPSE_ING) {
      DrawInfoPtr ptr_info = std::make_shared<DrawInfo>(DRAW_TYPE::DT_ELLIPSE,
                                                        start_pos_, end_pos_);
      send_remote_cmd(ptr_info);
    }
    draw(canvas_img_, false /*send_romote*/);
  }
}

void PainterDemo::paintEvent(QPaintEvent *event) {
  QPainter p(this);
  if (drawing_) {
    p.drawImage(0, 0, canvas_cache_img_);
  } else {
    p.drawImage(0, 0, canvas_img_);
  }
}

void PainterDemo::draw(QImage &img, bool send_remote) {
  if (img.isNull()) {
    return;
  }
  QPainter p(&img);
  p.setPen(qRgba(255, 0, 0, 255));
  p.setRenderHint(QPainter::Antialiasing, true);

  if (send_remote) {
    DrawInfoPtr ptr_info =
        std::make_shared<DrawInfo>(draw_type_, start_pos_, end_pos_);
    send_remote_cmd(ptr_info);
  }
  switch (draw_type_) {
  case DRAW_TYPE::DT_PEN: {
    p.drawLine(start_pos_, end_pos_);
    start_pos_ = end_pos_;
    break;
  }
  case DRAW_TYPE::DT_ELLIPSE_ING: {
    p.drawEllipse(QRect(start_pos_, end_pos_));
    break;
  }
  case DRAW_TYPE::DT_ELLIPSE: {
    p.drawEllipse(QRect(start_pos_, end_pos_));
    break;
  }
  case DRAW_TYPE::DT_UNDO: {
    break;
  }
  case DRAW_TYPE::DT_RE_UNDO: {
    break;
  }
  case DRAW_TYPE::DT_NEW_STROKE: {
    break;
  }
  default:
    break;
  }

  update();
}

void PainterDemo::start_new_stroke() {
  // 将当前画布加入撤销列表;
  push_undo_image(canvas_img_);
  // 有新的修改则不能反撤销;
  re_undo_lists_.clear();
  re_undo_btn_->setEnabled(false);
  send_remote_cmd(
      std::make_shared<DrawInfo>(DT_NEW_STROKE, start_pos_, end_pos_));
}

void PainterDemo::on_undo_clicked(bool checked) {
  push_re_undo_image(canvas_img_);
  canvas_img_ = *(undo_lists_.rbegin());
  undo_lists_.pop_back();
  if (undo_lists_.size() == 0) {
    undo_btn_->setEnabled(false);
  } else {
  }
  update();
  {
    DrawInfoPtr ptr_info =
        std::make_shared<DrawInfo>(DT_UNDO, start_pos_, end_pos_);
    send_remote_cmd(ptr_info);
  }
}

void PainterDemo::on_re_undo_clicked(bool checked) {
  push_undo_image(canvas_img_);
  canvas_img_ = *(re_undo_lists_.rbegin());
  re_undo_lists_.pop_back();
  if (re_undo_lists_.size() == 0) {
    re_undo_btn_->setEnabled(false);
  } else {
  }
  update();
  {
    DrawInfoPtr ptr_info =
        std::make_shared<DrawInfo>(DT_RE_UNDO, start_pos_, end_pos_);
    send_remote_cmd(ptr_info);
  }
}

void PainterDemo::on_ellipse_clicked(bool checked) {
  if (checked) {
    draw_type_ = DT_ELLIPSE_ING;
  } else {
    draw_type_ = DT_PEN;
  }
}

void PainterDemo::push_undo_image(const QImage &img) {
  if (undo_lists_.size() >= MAX_UNDO_COUNT) {
    undo_lists_.pop_front();
    // ADDED(gaoxincheng): 该处应保存基准图形，更新通知到服务端;
    // 后续加入端，基于该图形开始绘制;
  }
  undo_lists_.push_back(img);
  undo_btn_->setEnabled(true);
}

void PainterDemo::push_re_undo_image(const QImage &img) {
  re_undo_lists_.push_back(canvas_img_);
  re_undo_btn_->setEnabled(true);
}

void PainterDemo::send_remote_cmd(const DrawInfoPtr &ptr_info) {
  draw_cmd_list_.push_back(ptr_info);
  do_remote_cmd();
}

void PainterDemo::do_remote_cmd() {
  if (draw_cmd_list_.size() == 0) {
    return;
  }

  auto it = draw_cmd_list_.begin();
  for (; it != draw_cmd_list_.end(); ++it) {
    const DrawInfoPtr &ptr_info = *it;
    switch (ptr_info->draw_type) {
    case DRAW_TYPE::DT_PEN: {
      QPainter p(&remote_canvas_img_);
      p.setPen(qRgba(255, 0, 0, 255));
      p.setRenderHint(QPainter::Antialiasing, true);
      p.drawLine(ptr_info->start_pos, ptr_info->end_pos);
      remote_canvas_label_->setPixmap(QPixmap::fromImage(remote_canvas_img_));
      break;
    }
    case DRAW_TYPE::DT_ELLIPSE_ING: {
      remote_canvas_cache_img_ = remote_canvas_img_;
      QPainter p(&remote_canvas_cache_img_);
      p.setPen(qRgba(255, 0, 0, 255));
      p.setRenderHint(QPainter::Antialiasing, true);
      p.drawEllipse(QRect(ptr_info->start_pos, ptr_info->end_pos));
      remote_canvas_label_->setPixmap(
          QPixmap::fromImage(remote_canvas_cache_img_));
      break;
    }
    case DRAW_TYPE::DT_ELLIPSE: {
      QPainter p(&remote_canvas_img_);
      p.setPen(qRgba(255, 0, 0, 255));
      p.setRenderHint(QPainter::Antialiasing, true);
      p.drawEllipse(QRect(ptr_info->start_pos, ptr_info->end_pos));
      remote_canvas_label_->setPixmap(QPixmap::fromImage(remote_canvas_img_));
      break;
    }
    case DRAW_TYPE::DT_UNDO: {
      remote_re_undo_lists_.push_back(remote_canvas_img_);
      remote_canvas_img_ = *(remote_undo_lists_.rbegin());
      remote_undo_lists_.pop_back();
      remote_canvas_label_->setPixmap(QPixmap::fromImage(remote_canvas_img_));
      break;
    }
    case DRAW_TYPE::DT_RE_UNDO: {
      remote_undo_lists_.push_back(remote_canvas_img_);
      remote_canvas_img_ = *(remote_re_undo_lists_.rbegin());
      remote_re_undo_lists_.pop_back();
      remote_canvas_label_->setPixmap(QPixmap::fromImage(remote_canvas_img_));
      break;
    }
    case DRAW_TYPE::DT_NEW_STROKE: {
      if (remote_undo_lists_.size() >= MAX_UNDO_COUNT) {
        remote_undo_lists_.pop_front();
      }
      remote_undo_lists_.push_back(remote_canvas_img_);
      remote_canvas_label_->setPixmap(QPixmap::fromImage(remote_canvas_img_));
      break;
    }
    default:
      break;
    }
  }
  draw_cmd_list_.clear();

  remote_canvas_label_->show();
}
