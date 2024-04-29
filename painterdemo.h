#ifndef PAINTERDEMO_H
#define PAINTERDEMO_H

#include "ui_painterdemo.h"
#include <QImage>
#include <QPaintEvent>
#include <QPoint>
#include <QtGui/QMainWindow>
class QPushButton;
class QTimer;
class QLabel;
// 最大撤销次数;
const int MAX_UNDO_COUNT = 5;

enum DRAW_TYPE {
  DT_PEN,         // 画笔;
  DT_ELLIPSE_ING, // 椭圆绘制中状态，接收端绘制到缓存,实现实时更新效果;
  DT_ELLIPSE,     // 椭圆绘制结束，绘制到真正的图片中;
  DT_UNDO,        // 撤销;
  DT_RE_UNDO,     // 反撤销;
  DT_NEW_STROKE,  // 新的一笔;
};

struct DrawInfo {
  int draw_type;
  QPoint start_pos;
  QPoint end_pos;
  DrawInfo(int dt, QPoint s, QPoint e) {
    draw_type = dt;
    start_pos = s;
    end_pos = e;
  }
  DrawInfo(const DrawInfo &other) {
    if (this == &other) {
      return;
    }
    *this = other;
  }
  const DrawInfo &operator=(const DrawInfo &other) {
    this->draw_type = other.draw_type;
    this->start_pos = other.start_pos;
    this->end_pos = other.end_pos;
    return *this;
  }
};
typedef std::shared_ptr<DrawInfo> DrawInfoPtr;

class PainterDemo : public QMainWindow {
  Q_OBJECT

public:
  PainterDemo(QWidget *parent = 0, Qt::WFlags flags = 0);
  ~PainterDemo();

public:
  void init();

protected:
  virtual void mousePressEvent(QMouseEvent *event);
  virtual void mouseMoveEvent(QMouseEvent *event);
  virtual void mouseReleaseEvent(QMouseEvent *event);
  virtual void paintEvent(QPaintEvent *event);

public slots:
  void on_undo_clicked(bool checked);
  void on_re_undo_clicked(bool checked);
  void on_ellipse_clicked(bool checked);

private:
  void draw(QImage &img, bool send_remote = true);

  void start_new_stroke();

  void push_undo_image(const QImage &img);
  void push_re_undo_image(const QImage &img);

  void send_remote_cmd(const DrawInfoPtr &ptr_info);
  void do_remote_cmd();

  // local canvas info;
private:
  QPoint start_pos_;
  QPoint end_pos_;
  bool drawing_;
  int draw_type_;
  QImage canvas_img_;
  QImage canvas_cache_img_;

  bool new_draw_;
  std::list<QImage> undo_lists_;
  std::list<QImage> re_undo_lists_;

  // mock for remote canvas info;
private:
  QLabel *remote_canvas_label_;
  QImage remote_canvas_img_;
  QImage remote_canvas_cache_img_;

  std::list<DrawInfoPtr> draw_cmd_list_;
  std::list<QImage> remote_undo_lists_;
  std::list<QImage> remote_re_undo_lists_;

private:
  Ui::PainterDemoClass ui;

  QPushButton *ellipse_btn_;

  QPushButton *undo_btn_;
  QPushButton *re_undo_btn_;
};

#endif // PAINTERDEMO_H
