/*
 * Partial DokuWiki markup parser/renderer
 * == h5 ==
 * ==== h3 ====
 * ====== h1 ======
 * [[http://some.web.site/|some link text]]
 * {{http://some.web.site/image.png|some alt text}}
 * ** bold **
 * // italic //
 * __ underline __
 * > blockquote
 * * Unordered lists
 *  * With sub-items
 * - Ordered lists
 *  - With sub-items
 * To do:
 *  - nested blockquote in HTML
 *  - image links
 *  - DokuWiki-style tables
 *  - code blocks
 *  - text conversion (HTML only probably)
 */
load('sbbsdefs.js');

if (typeof Frame == 'undefined') Frame = false;

function Markdown(target, settings) {

  const state = {
    list_level : 0,
    links : [],
    images : [],
    footnotes : [],
    table : [],
    blockquote : false,
    list_stack : []
  };

  const config = {
    console : {
      bold_style : '\1h',
      italic_style : '\1r',
      underline_style : '\1g',
      list_indent : '\t',
      heading_underline : true,
      heading_style : '\1h',
      link_style : '\1h\1c',
      image_style : '\1h\1m',
      footnote_style : '\1h\1y'
    },
    html : {
      a : '',
      ul : 'list-group',
      ol : 'list-group',
      li : 'list-group-item',
      table : 'table table-striped',
      thead : '',
      tbody : '',
      th : '',
      tr : '',
      td : '',
      img : '',
      hr : '',
      blockquote : 'blockquote'
    }
  };
  if (typeof settings == 'object') {
    if (typeof settings.console == 'object') {
      Object.keys(settings.console).forEach(function (e) {
        config.console[e] = settings.console[e];
      });
    }
    if (typeof settings.html == 'object') {
      Object.keys(settings.html).forEach(function (e) {
        config.html[e] = settings.html[e];
      });
    }
  }

  if (Frame && target instanceof Frame) target.word_wrap = true;

  this.reset = function () {
    state.list_level = 0;
    state.links = [];
    state.images = [];
    state.footnotes = [];
    state.table = [];
    state.blockquote = false;
    state.list_stack = [];
  }

  Object.defineProperty(this, 'state', { get : function () {
    return state;
  }});

  Object.defineProperty(this, 'target', {
    get : function () {
      return target;
    },
    set : function (t) {
      this.reset();
      if (t == 'html') {
        target = t;
      } else if (Frame && t instanceof Frame) {
        target = t;
      } else if (
        typeof t.screen_columns == 'number' && typeof t.putmsg == 'function'
      ) {
        target = t;
      } else {
        throw 'Invalid output target';
      }
    }
  });

  Object.defineProperty(this, 'columns', { get : function () {
    if (target == 'html') {
      return 0;
    } else if (Frame && target instanceof Frame) {
      return target.width;
    } else {
      return target.screen_columns;
    }
  }});

  Object.defineProperty(this, 'config', { value : config });

}

Markdown.prototype.html_tag_format = function (tag, attributes) {
  var ret = '<' + tag;
  if (this.config.html[tag] != '') {
    ret += ' class="' + this.config.html[tag] + '"';
  }
  if (attributes) {
    Object.keys(attributes).forEach(function (e) {
      ret += ' ' + e + '="' + attributes[e] + '"'
    });
  }
  return ret + '>';
}

Markdown.prototype.render_text_console = function (text) {
  const self = this;
  return text.replace(/\*\*([^\*]+)\*\*/g, function (m, c) {
    return '\1+' + self.config.console.bold_style + c + '\1-';
  }).replace(/\/\/([^\/]+)\/\//g, function (m, c) {
    return '\1+' + self.config.console.italic_style + c + '\1-';
  }).replace(/__([^_]+)__/g, function (m, c) {
    return '\1+' + self.config.console.underline_style + c + '\1-';
  }).replace(/''([^']+)''/g, function (m, c) {
    return c;
  }).replace(/\{\{(.+)\}\}/g, function (m, c) {
    c = c.split('|');
    self.state.images.push({ text : (c[1] || c[0]), link : c[0] });
    return '\1+' + self.config.console.image_style + (c[1] || c[0]) + ' [' + self.state.images.length + ']\1-';
  }).replace(/\[\[([^\]]+)\]\]/g, function (m, c) {
    c = c.split('|');
    self.state.links.push({ text : c[1] || c[0], link : c[0] });
    return '\1+' + self.config.console.link_style + (c[1] || c[0]) + ' [' + self.state.links.length + ']\1-';
  }).replace(/\(\(([^\)]+)\)\)/g, function (m, c) {
    self.state.footnotes.push(c);
    return '\1+' + self.config.console.footnote_style + '[' + self.state.footnotes.length + ']\1-';
  });
}

Markdown.prototype.render_text_html = function (text) {
  const self = this;
  return text.replace(/\\1.(.+)\\1./g, function (m, c) {
    return c;
  }).replace(/\*\*([^\*]+)\*\*/g, function (m, c) {
    return '<b>' + c + '</b>';
  }).replace(/\/\/([^\/]+)\/\//g, function (m, c) {
    return '<i>' + c + '</i>';
  }).replace(/__([^_]+)__/g, function (m, c) {
    return '<span style="text-decoration:underline;">' + c + '</span>';
  }).replace(/''([^']+)''/g, function (m, c) {
    return '<code>' + c + '</code>';
  }).replace(/\{\{(.+)\}\}/g, function (m, c) {
    c = c.split('|');
    return self.html_tag_format('img', { alt : (c[1] || c[0]), src : c[0] });
  }).replace(/\[\[([^\]]+)\]\]/g, function (m, c) {
    c = c.split('|');
    return self.html_tag_format('a', { href : c[0] }) + (c[1] || c[0]) + '</a>';
  }).replace(/\(\(([^\)]+)\)\)/g, function (m, c) {
    self.state.footnotes.push(c);
    return self.html_tag_format('a', { href : '#f' + self.state.footnotes.length }) + ' [' + self.state.footnotes.length + ']</a>';
  });
}

Markdown.prototype.render_table = function () {

  const self = this;
  const columns = []; // Length is number of columns, values are column widths
  this.state.table.forEach(function (e) {
    e.forEach(function (e, i) {
      const raw = strip_ctrl(e);
      const visible = raw ? raw.length : 0;
      if (columns.length < (i + 1)) {
        columns.push(visible);
      } else if (columns[i] < visible) {
        columns[i] = visible;
      }
    });
  });
  if (this.target == 'html') {
    var ret = this.html_tag_format('table');
    this.state.table.forEach(function (e, i, a) {
      ret += self.html_tag_format('tr');
      if (i == 0) ret += self.html_tag_format('thead');
      for (var n = 0; n < columns.length; n++) {
        if (e[n] == ':::') continue;
        if (e[n] == '') continue;
        var attr = {};
        var nr = i + 1;
        if (self.state.table[nr] && self.state.table[nr][n] == ':::') {
          attr.rowspan = 1;
          while (
            typeof self.state.table[nr] !== 'undefined'
            && self.state.table[nr][n] == ':::'
          ) {
            attr.rowspan++;
            nr++;
          }
          if (attr.rowspan < 2) delete attr.rowspan;
        }
        var nc = n + 1;
        if (typeof e[nc] != 'undefined' && e[nc] == '') {
          log('here');
          attr.colspan = 1;
          while (typeof e[nc] !== 'undefined' && e[nc] == '') {
            attr.colspan++;
            nc++;
          }
          if (attr.colspan < 2) delete attr.colspan;
        }
        var tt = i == 0 ? 'th' : 'td';
        var tag = [self.html_tag_format(tt, attr), '</' + tt + '/>'];
        ret += tag[0] + e[n] + tag[1];
      }
      ret += '</tr>';
      if (i == 0) ret += '</thead>';
    });
    ret += '</table><br>';
    this.state.table = [];
    return ret;
  } else {
    // This is pretty bad, but doing it right will be annoying
    // There is no wrapping of cell contents; long lines just get truncated
    var ret = [];
    this.state.table.forEach(function (e, i) {
      var out = '| ';
      for (var n = 0; n < columns.length; n++) {
        var s = e.length < n + 1 ? ' ' : (e[n] == '' ? ' ' : e[n]);
        while (strip_ctrl(s).length < columns[n]) {
          s += ' ';
        }
        out += s + ' | ';
      }
      while (strip_ctrl(out).length > self.columns - 2) {
        out = out.substring(0, out.length - 1);
      }
      ret.push(out);
      if (i == 0) {
        out = '|-';
        for (var n = 0; n < columns.length; n++) {
          var s = '';
          while (s.length < columns[n]) {
            s += '-';
          }
          out += s + '-|';
          if (n < columns.length - 1) out += '-';
        }
        while (strip_ctrl(out).length > self.columns - 2) {
          out = out.substring(0, out.length - 1);
        }
        ret.push(out);
      }
    });
    this.state.table = [];
    return ret.join('\r\n') + '\r\n';
  }

}

Markdown.prototype.render_line_console = function (line) {

  var match;
  const self = this;
  var ret = this.render_text_console(line);

  // Ordered and unordered lists
  match = ret.match(/^(\s*)(\*|-)\s+(.+)$/);
  if (match !== null) {
    ret = ret.replace(match[0], '');
    if (this.state.table.length) ret += this.render_table();
    if (match[2] == '*') {
      lt = 'ul';
    } else {
      lt = 'ol';
    }
    if (match[1].length > this.state.list_level) {
      this.state.list_level++;
      if (lt == 'ol') this.state.list_stack.push(0);
    } else if (match[1].length < this.state.list_level) {
      this.state.list_level--;
      if (lt == 'ol') this.state.list_stack.pop();
    } else if (lt == 'ol') {
      if (typeof this.state.list_stack[this.state.list_level] != 'number') {
        this.state.list_stack[this.state.list_level] = 0;
      } else {
        this.state.list_stack[this.state.list_level]++;
      }
    }
    for (var n = 0; n < this.state.list_level; n++) {
      ret += this.config.console.list_indent;
    }
    if (lt == 'ul') {
      ret += match[2];
    } else {
      ret += (this.state.list_stack[this.state.list_level] + 1) + '.';
    }
    ret += ' ' + match[3] + '\r\n';
    return ret;
  }
  if (this.state.list_level) {
    ret += '\r\n';
    this.state.list_level = 0;
    this.state.list_stack = [];
  }

  // Table
  const tre = /([|^])([^|^]+)(?=[|^])/g;
  match = tre.exec(ret);
  if (match !== null) {
    const _ret = match.input;
    const row = [];
    do {
      if (match[1] == '^') {
        row.push(
          '\1+' + this.config.console.heading_style + match[2] + '\1-'
        );
      } else {
        row.push(match[2]);
      }
      match = tre.exec(ret);
    } while (match !== null);
    this.state.table.push(row);
    ret = ret.replace(_ret, '');
    return ret;
  } else if (this.state.table.length) {
    ret += this.render_table();
  }

  // Heading
  match = ret.match(/^(==+)\s(.+)\s==+$/);
  if (match !== null) {
    ret = ret.replace(match[0], '');
    ret += '\1+';
    ret += this.config.console.heading_style;
    ret += match[2];
    if (this.config.console.heading_underline) {
      ret += '\r\n';
      for (var n = 0; n < match[2].length; n++) {
        ret += user.settings&USER_NO_EXASCII ? '-' : ascii(196);
      }
    }
    ret += '\1-\r\n\r\n';
    return ret;
  }

  // Blockquote
  match = ret.match(/^\s*>\s(.+)$/);
  if (match !== null) {
    return ret.replace(
      match[0], quote_msg(word_wrap(match[1]), this.columns - 1)
    ) + '\r\n';
  }

  // Horizontal Rule
  match = ret.match(/^----+$/);
  if (match !== null) {
    var s = '';
    while (s.length < this.columns - 1) {
      s += user.settings&USER_NO_EXASCII ? '-' : ascii(196);
    }
    return ret.replace(match[0], s) + '\r\n';
  }

  return ret + '\r\n';

}

Markdown.prototype.render_line_html = function (line) {

  var match;
  const self = this;
  var ret = this.render_text_html(line);

  // Blockquote
  match = ret.match(/^\s*>\s(.+)$/);
  if (match !== null) {
    ret = ret.replace(match[0], '');
    if (this.state.table.length) ret += this.render_table();
    if (!this.state.blockquote) {
      ret += this.html_tag_format('blockquote');
      this.state.blockquote = true;
    }
    return ret + match[1];
  } else if (this.state.blockquote) {
    ret += '</blockquote>';
    this.state.blockquote = false;
  }

  // Ordered and unordered lists
  match = ret.match(/^(\s*)(\*|-)\s+(.+)$/);
  if (match !== null) {
    ret = ret.replace(match[0], '');
    if (this.state.table.length) ret += this.render_table();
    var lt = (match[2] == '*' ? 'ul' : 'ol');
    if (!match[1].length) {
      while (this.state.list_stack.length > 1) {
        ret += '</' + this.state.list_stack.pop() + '></li>';
      }
      if (this.state.list_stack.length < 1) {
        this.state.list_stack.push(lt);
        ret += this.html_tag_format(lt);
      }
    } else if (match[1].length >= this.state.list_stack.length) {
      this.state.list_stack.push(lt);
      ret += this.html_tag_format('li');
      ret += this.html_tag_format(lt);
    }
    ret += this.html_tag_format('li');
    ret += match[3];
    ret += '</li>';
    return ret;
  }
  while (this.state.list_stack.length) {
    ret += '</' + this.state.list_stack.pop() + '>';
  }

  // Table
  const tre = /([|^])([^|^]*)(?=[|^])/g;
  match = tre.exec(ret);
  if (match !== null) {
    const _ret = match.input;
    const row = [];
    do {
      if (match[1] == '^') {
        // This is lousy, but if you want table headings to look special,
        // then include a 'doku_th' class in your stylesheet.
        // You're welcome.
        row.push('<span class="doku_th">' + match[2] + '</span>');
      } else {
        row.push(match[2]);
      }
      match = tre.exec(ret);
    } while (match !== null);
    this.state.table.push(row);
    log('row ' + JSON.stringify(row));
    ret = ret.replace(_ret, '');
    return ret;
  } else if (this.state.table.length) {
    ret += this.render_table();
  }

  // Heading
  match = ret.match(/^(==+)\s(.+)\s==+$/);
  if (match !== null) {
    ret = ret.replace(match[0], '');
    var lvl = 6 - Math.min(match[1].split(' ')[0].length, 5);
    ret += '<h' + lvl + '>';
    ret += match[2];
    ret += '</h' + lvl + '>';
    return ret;
  }

  // Horizontal Rule
  match = ret.match(/^----+$/);
  if (match !== null) {
    return ret.replace(match[0], '') + this.html_tag_format('hr');
  }

  return ret + '<br>';

}

Markdown.prototype.render_console = function (text) {
  const self = this;
  text.split(/\n/).forEach(function (e) {
    var line = self.render_line_console(e.replace(/\r$/, ''));
    if (typeof line == 'string') {
      self.target.putmsg(line);//.substr(0, self.columns));
    }
  });
  if (this.state.links.length) {
    this.target.putmsg('\1+' + self.config.console.link_style + 'Links:\1-\r\n');
    this.state.links.forEach(function (e, i) {
      self.target.putmsg('\1+' + self.config.console.link_style + '[' + (i + 1) + '] ' + e.link + '\1-\r\n');
    });
    this.target.putmsg('\r\n');
  }
  if (this.state.images.length) {
    this.target.putmsg('\1+' + self.config.console.image_style + 'Images:\1-\r\n');
    this.state.images.forEach(function (e, i) {
      self.target.putmsg('\1+' + self.config.console.image_style + '[' + (i + 1) + '] ' + e.link + '\1-\r\n');
    });
    this.target.putmsg('\r\n');
  }
  if (this.state.footnotes.length) {
    this.target.putmsg('\1+' + self.config.console.footnote_style + 'Footnotes:\1-\r\n');
    this.state.footnotes.forEach(function (e, i) {
      self.target.putmsg('\1+' + self.config.console.footnote_style + '[' + (i + 1) + '] ' + e + '\1-\r\n');
    });
  }
}

Markdown.prototype.render_html = function (text) {
  const self = this;
  text.split(/\n/).forEach(function (e) {
    var line = self.render_line_html(e.replace(/\r$/, ''));
    if (typeof line == 'string') writeln(line);
  });
  if (this.state.footnotes.length) {
    writeln('<hr>Footnotes:<br>');
    this.state.footnotes.forEach(function (e, i) {
      writeln('<a id="f' + (i + 1) + '">[' + (i + 1) + '] ' + e + '</a><br>');
    });
  }
}

Markdown.prototype.render = function (text) {
  if (this.target == 'html') {
    this.render_html(text);
  } else {
    this.render_console(text);
  }
}
