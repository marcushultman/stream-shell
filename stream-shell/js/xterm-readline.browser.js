var __create = Object.create;
var __defProp = Object.defineProperty;
var __getOwnPropDesc = Object.getOwnPropertyDescriptor;
var __getOwnPropNames = Object.getOwnPropertyNames;
var __getProtoOf = Object.getPrototypeOf;
var __hasOwnProp = Object.prototype.hasOwnProperty;
var __commonJS = (cb, mod) => function __require() {
  return mod || (0, cb[__getOwnPropNames(cb)[0]])((mod = { exports: {} }).exports, mod), mod.exports;
};
var __copyProps = (to, from, except, desc) => {
  if (from && typeof from === "object" || typeof from === "function") {
    for (let key of __getOwnPropNames(from))
      if (!__hasOwnProp.call(to, key) && key !== except)
        __defProp(to, key, { get: () => from[key], enumerable: !(desc = __getOwnPropDesc(from, key)) || desc.enumerable });
  }
  return to;
};
var __toESM = (mod, isNodeMode, target) => (target = mod != null ? __create(__getProtoOf(mod)) : {}, __copyProps(
  // If the importer is in node compatibility mode or this is not an ESM
  // file that has been converted to a CommonJS file using a Babel-
  // compatible transform (i.e. "__esModule" has not been set), then set
  // "default" to the CommonJS "module.exports" for node compatibility.
  isNodeMode || !mod || !mod.__esModule ? __defProp(target, "default", { value: mod, enumerable: true }) : target,
  mod
));

// node_modules/ansi-regex/index.js
var require_ansi_regex = __commonJS({
  "node_modules/ansi-regex/index.js"(exports, module) {
    "use strict";
    module.exports = ({ onlyFirst = false } = {}) => {
      const pattern = [
        "[\\u001B\\u009B][[\\]()#;?]*(?:(?:(?:(?:;[-a-zA-Z\\d\\/#&.:=?%@~_]+)*|[a-zA-Z\\d]+(?:;[-a-zA-Z\\d\\/#&.:=?%@~_]*)*)?\\u0007)",
        "(?:(?:\\d{1,4}(?:;\\d{0,4})*)?[\\dA-PR-TZcf-ntqry=><~]))"
      ].join("|");
      return new RegExp(pattern, onlyFirst ? void 0 : "g");
    };
  }
});

// node_modules/strip-ansi/index.js
var require_strip_ansi = __commonJS({
  "node_modules/strip-ansi/index.js"(exports, module) {
    "use strict";
    var ansiRegex = require_ansi_regex();
    module.exports = (string) => typeof string === "string" ? string.replace(ansiRegex(), "") : string;
  }
});

// node_modules/is-fullwidth-code-point/index.js
var require_is_fullwidth_code_point = __commonJS({
  "node_modules/is-fullwidth-code-point/index.js"(exports, module) {
    "use strict";
    var isFullwidthCodePoint = (codePoint) => {
      if (Number.isNaN(codePoint)) {
        return false;
      }
      if (codePoint >= 4352 && (codePoint <= 4447 || // Hangul Jamo
      codePoint === 9001 || // LEFT-POINTING ANGLE BRACKET
      codePoint === 9002 || // RIGHT-POINTING ANGLE BRACKET
      // CJK Radicals Supplement .. Enclosed CJK Letters and Months
      11904 <= codePoint && codePoint <= 12871 && codePoint !== 12351 || // Enclosed CJK Letters and Months .. CJK Unified Ideographs Extension A
      12880 <= codePoint && codePoint <= 19903 || // CJK Unified Ideographs .. Yi Radicals
      19968 <= codePoint && codePoint <= 42182 || // Hangul Jamo Extended-A
      43360 <= codePoint && codePoint <= 43388 || // Hangul Syllables
      44032 <= codePoint && codePoint <= 55203 || // CJK Compatibility Ideographs
      63744 <= codePoint && codePoint <= 64255 || // Vertical Forms
      65040 <= codePoint && codePoint <= 65049 || // CJK Compatibility Forms .. Small Form Variants
      65072 <= codePoint && codePoint <= 65131 || // Halfwidth and Fullwidth Forms
      65281 <= codePoint && codePoint <= 65376 || 65504 <= codePoint && codePoint <= 65510 || // Kana Supplement
      110592 <= codePoint && codePoint <= 110593 || // Enclosed Ideographic Supplement
      127488 <= codePoint && codePoint <= 127569 || // CJK Unified Ideographs Extension B .. Tertiary Ideographic Plane
      131072 <= codePoint && codePoint <= 262141)) {
        return true;
      }
      return false;
    };
    module.exports = isFullwidthCodePoint;
    module.exports.default = isFullwidthCodePoint;
  }
});

// node_modules/emoji-regex/index.js
var require_emoji_regex = __commonJS({
  "node_modules/emoji-regex/index.js"(exports, module) {
    "use strict";
    module.exports = function() {
      return /\uD83C\uDFF4\uDB40\uDC67\uDB40\uDC62(?:\uDB40\uDC65\uDB40\uDC6E\uDB40\uDC67|\uDB40\uDC73\uDB40\uDC63\uDB40\uDC74|\uDB40\uDC77\uDB40\uDC6C\uDB40\uDC73)\uDB40\uDC7F|\uD83D\uDC68(?:\uD83C\uDFFC\u200D(?:\uD83E\uDD1D\u200D\uD83D\uDC68\uD83C\uDFFB|\uD83C[\uDF3E\uDF73\uDF93\uDFA4\uDFA8\uDFEB\uDFED]|\uD83D[\uDCBB\uDCBC\uDD27\uDD2C\uDE80\uDE92]|\uD83E[\uDDAF-\uDDB3\uDDBC\uDDBD])|\uD83C\uDFFF\u200D(?:\uD83E\uDD1D\u200D\uD83D\uDC68(?:\uD83C[\uDFFB-\uDFFE])|\uD83C[\uDF3E\uDF73\uDF93\uDFA4\uDFA8\uDFEB\uDFED]|\uD83D[\uDCBB\uDCBC\uDD27\uDD2C\uDE80\uDE92]|\uD83E[\uDDAF-\uDDB3\uDDBC\uDDBD])|\uD83C\uDFFE\u200D(?:\uD83E\uDD1D\u200D\uD83D\uDC68(?:\uD83C[\uDFFB-\uDFFD])|\uD83C[\uDF3E\uDF73\uDF93\uDFA4\uDFA8\uDFEB\uDFED]|\uD83D[\uDCBB\uDCBC\uDD27\uDD2C\uDE80\uDE92]|\uD83E[\uDDAF-\uDDB3\uDDBC\uDDBD])|\uD83C\uDFFD\u200D(?:\uD83E\uDD1D\u200D\uD83D\uDC68(?:\uD83C[\uDFFB\uDFFC])|\uD83C[\uDF3E\uDF73\uDF93\uDFA4\uDFA8\uDFEB\uDFED]|\uD83D[\uDCBB\uDCBC\uDD27\uDD2C\uDE80\uDE92]|\uD83E[\uDDAF-\uDDB3\uDDBC\uDDBD])|\u200D(?:\u2764\uFE0F\u200D(?:\uD83D\uDC8B\u200D)?\uD83D\uDC68|(?:\uD83D[\uDC68\uDC69])\u200D(?:\uD83D\uDC66\u200D\uD83D\uDC66|\uD83D\uDC67\u200D(?:\uD83D[\uDC66\uDC67]))|\uD83D\uDC66\u200D\uD83D\uDC66|\uD83D\uDC67\u200D(?:\uD83D[\uDC66\uDC67])|(?:\uD83D[\uDC68\uDC69])\u200D(?:\uD83D[\uDC66\uDC67])|[\u2695\u2696\u2708]\uFE0F|\uD83D[\uDC66\uDC67]|\uD83C[\uDF3E\uDF73\uDF93\uDFA4\uDFA8\uDFEB\uDFED]|\uD83D[\uDCBB\uDCBC\uDD27\uDD2C\uDE80\uDE92]|\uD83E[\uDDAF-\uDDB3\uDDBC\uDDBD])|(?:\uD83C\uDFFB\u200D[\u2695\u2696\u2708]|\uD83C\uDFFF\u200D[\u2695\u2696\u2708]|\uD83C\uDFFE\u200D[\u2695\u2696\u2708]|\uD83C\uDFFD\u200D[\u2695\u2696\u2708]|\uD83C\uDFFC\u200D[\u2695\u2696\u2708])\uFE0F|\uD83C\uDFFB\u200D(?:\uD83C[\uDF3E\uDF73\uDF93\uDFA4\uDFA8\uDFEB\uDFED]|\uD83D[\uDCBB\uDCBC\uDD27\uDD2C\uDE80\uDE92]|\uD83E[\uDDAF-\uDDB3\uDDBC\uDDBD])|\uD83C[\uDFFB-\uDFFF])|(?:\uD83E\uDDD1\uD83C\uDFFB\u200D\uD83E\uDD1D\u200D\uD83E\uDDD1|\uD83D\uDC69\uD83C\uDFFC\u200D\uD83E\uDD1D\u200D\uD83D\uDC69)\uD83C\uDFFB|\uD83E\uDDD1(?:\uD83C\uDFFF\u200D\uD83E\uDD1D\u200D\uD83E\uDDD1(?:\uD83C[\uDFFB-\uDFFF])|\u200D\uD83E\uDD1D\u200D\uD83E\uDDD1)|(?:\uD83E\uDDD1\uD83C\uDFFE\u200D\uD83E\uDD1D\u200D\uD83E\uDDD1|\uD83D\uDC69\uD83C\uDFFF\u200D\uD83E\uDD1D\u200D(?:\uD83D[\uDC68\uDC69]))(?:\uD83C[\uDFFB-\uDFFE])|(?:\uD83E\uDDD1\uD83C\uDFFC\u200D\uD83E\uDD1D\u200D\uD83E\uDDD1|\uD83D\uDC69\uD83C\uDFFD\u200D\uD83E\uDD1D\u200D\uD83D\uDC69)(?:\uD83C[\uDFFB\uDFFC])|\uD83D\uDC69(?:\uD83C\uDFFE\u200D(?:\uD83E\uDD1D\u200D\uD83D\uDC68(?:\uD83C[\uDFFB-\uDFFD\uDFFF])|\uD83C[\uDF3E\uDF73\uDF93\uDFA4\uDFA8\uDFEB\uDFED]|\uD83D[\uDCBB\uDCBC\uDD27\uDD2C\uDE80\uDE92]|\uD83E[\uDDAF-\uDDB3\uDDBC\uDDBD])|\uD83C\uDFFC\u200D(?:\uD83E\uDD1D\u200D\uD83D\uDC68(?:\uD83C[\uDFFB\uDFFD-\uDFFF])|\uD83C[\uDF3E\uDF73\uDF93\uDFA4\uDFA8\uDFEB\uDFED]|\uD83D[\uDCBB\uDCBC\uDD27\uDD2C\uDE80\uDE92]|\uD83E[\uDDAF-\uDDB3\uDDBC\uDDBD])|\uD83C\uDFFB\u200D(?:\uD83E\uDD1D\u200D\uD83D\uDC68(?:\uD83C[\uDFFC-\uDFFF])|\uD83C[\uDF3E\uDF73\uDF93\uDFA4\uDFA8\uDFEB\uDFED]|\uD83D[\uDCBB\uDCBC\uDD27\uDD2C\uDE80\uDE92]|\uD83E[\uDDAF-\uDDB3\uDDBC\uDDBD])|\uD83C\uDFFD\u200D(?:\uD83E\uDD1D\u200D\uD83D\uDC68(?:\uD83C[\uDFFB\uDFFC\uDFFE\uDFFF])|\uD83C[\uDF3E\uDF73\uDF93\uDFA4\uDFA8\uDFEB\uDFED]|\uD83D[\uDCBB\uDCBC\uDD27\uDD2C\uDE80\uDE92]|\uD83E[\uDDAF-\uDDB3\uDDBC\uDDBD])|\u200D(?:\u2764\uFE0F\u200D(?:\uD83D\uDC8B\u200D(?:\uD83D[\uDC68\uDC69])|\uD83D[\uDC68\uDC69])|\uD83C[\uDF3E\uDF73\uDF93\uDFA4\uDFA8\uDFEB\uDFED]|\uD83D[\uDCBB\uDCBC\uDD27\uDD2C\uDE80\uDE92]|\uD83E[\uDDAF-\uDDB3\uDDBC\uDDBD])|\uD83C\uDFFF\u200D(?:\uD83C[\uDF3E\uDF73\uDF93\uDFA4\uDFA8\uDFEB\uDFED]|\uD83D[\uDCBB\uDCBC\uDD27\uDD2C\uDE80\uDE92]|\uD83E[\uDDAF-\uDDB3\uDDBC\uDDBD]))|\uD83D\uDC69\u200D\uD83D\uDC69\u200D(?:\uD83D\uDC66\u200D\uD83D\uDC66|\uD83D\uDC67\u200D(?:\uD83D[\uDC66\uDC67]))|(?:\uD83E\uDDD1\uD83C\uDFFD\u200D\uD83E\uDD1D\u200D\uD83E\uDDD1|\uD83D\uDC69\uD83C\uDFFE\u200D\uD83E\uDD1D\u200D\uD83D\uDC69)(?:\uD83C[\uDFFB-\uDFFD])|\uD83D\uDC69\u200D\uD83D\uDC66\u200D\uD83D\uDC66|\uD83D\uDC69\u200D\uD83D\uDC69\u200D(?:\uD83D[\uDC66\uDC67])|(?:\uD83D\uDC41\uFE0F\u200D\uD83D\uDDE8|\uD83D\uDC69(?:\uD83C\uDFFF\u200D[\u2695\u2696\u2708]|\uD83C\uDFFE\u200D[\u2695\u2696\u2708]|\uD83C\uDFFC\u200D[\u2695\u2696\u2708]|\uD83C\uDFFB\u200D[\u2695\u2696\u2708]|\uD83C\uDFFD\u200D[\u2695\u2696\u2708]|\u200D[\u2695\u2696\u2708])|(?:(?:\u26F9|\uD83C[\uDFCB\uDFCC]|\uD83D\uDD75)\uFE0F|\uD83D\uDC6F|\uD83E[\uDD3C\uDDDE\uDDDF])\u200D[\u2640\u2642]|(?:\u26F9|\uD83C[\uDFCB\uDFCC]|\uD83D\uDD75)(?:\uD83C[\uDFFB-\uDFFF])\u200D[\u2640\u2642]|(?:\uD83C[\uDFC3\uDFC4\uDFCA]|\uD83D[\uDC6E\uDC71\uDC73\uDC77\uDC81\uDC82\uDC86\uDC87\uDE45-\uDE47\uDE4B\uDE4D\uDE4E\uDEA3\uDEB4-\uDEB6]|\uD83E[\uDD26\uDD37-\uDD39\uDD3D\uDD3E\uDDB8\uDDB9\uDDCD-\uDDCF\uDDD6-\uDDDD])(?:(?:\uD83C[\uDFFB-\uDFFF])\u200D[\u2640\u2642]|\u200D[\u2640\u2642])|\uD83C\uDFF4\u200D\u2620)\uFE0F|\uD83D\uDC69\u200D\uD83D\uDC67\u200D(?:\uD83D[\uDC66\uDC67])|\uD83C\uDFF3\uFE0F\u200D\uD83C\uDF08|\uD83D\uDC15\u200D\uD83E\uDDBA|\uD83D\uDC69\u200D\uD83D\uDC66|\uD83D\uDC69\u200D\uD83D\uDC67|\uD83C\uDDFD\uD83C\uDDF0|\uD83C\uDDF4\uD83C\uDDF2|\uD83C\uDDF6\uD83C\uDDE6|[#\*0-9]\uFE0F\u20E3|\uD83C\uDDE7(?:\uD83C[\uDDE6\uDDE7\uDDE9-\uDDEF\uDDF1-\uDDF4\uDDF6-\uDDF9\uDDFB\uDDFC\uDDFE\uDDFF])|\uD83C\uDDF9(?:\uD83C[\uDDE6\uDDE8\uDDE9\uDDEB-\uDDED\uDDEF-\uDDF4\uDDF7\uDDF9\uDDFB\uDDFC\uDDFF])|\uD83C\uDDEA(?:\uD83C[\uDDE6\uDDE8\uDDEA\uDDEC\uDDED\uDDF7-\uDDFA])|\uD83E\uDDD1(?:\uD83C[\uDFFB-\uDFFF])|\uD83C\uDDF7(?:\uD83C[\uDDEA\uDDF4\uDDF8\uDDFA\uDDFC])|\uD83D\uDC69(?:\uD83C[\uDFFB-\uDFFF])|\uD83C\uDDF2(?:\uD83C[\uDDE6\uDDE8-\uDDED\uDDF0-\uDDFF])|\uD83C\uDDE6(?:\uD83C[\uDDE8-\uDDEC\uDDEE\uDDF1\uDDF2\uDDF4\uDDF6-\uDDFA\uDDFC\uDDFD\uDDFF])|\uD83C\uDDF0(?:\uD83C[\uDDEA\uDDEC-\uDDEE\uDDF2\uDDF3\uDDF5\uDDF7\uDDFC\uDDFE\uDDFF])|\uD83C\uDDED(?:\uD83C[\uDDF0\uDDF2\uDDF3\uDDF7\uDDF9\uDDFA])|\uD83C\uDDE9(?:\uD83C[\uDDEA\uDDEC\uDDEF\uDDF0\uDDF2\uDDF4\uDDFF])|\uD83C\uDDFE(?:\uD83C[\uDDEA\uDDF9])|\uD83C\uDDEC(?:\uD83C[\uDDE6\uDDE7\uDDE9-\uDDEE\uDDF1-\uDDF3\uDDF5-\uDDFA\uDDFC\uDDFE])|\uD83C\uDDF8(?:\uD83C[\uDDE6-\uDDEA\uDDEC-\uDDF4\uDDF7-\uDDF9\uDDFB\uDDFD-\uDDFF])|\uD83C\uDDEB(?:\uD83C[\uDDEE-\uDDF0\uDDF2\uDDF4\uDDF7])|\uD83C\uDDF5(?:\uD83C[\uDDE6\uDDEA-\uDDED\uDDF0-\uDDF3\uDDF7-\uDDF9\uDDFC\uDDFE])|\uD83C\uDDFB(?:\uD83C[\uDDE6\uDDE8\uDDEA\uDDEC\uDDEE\uDDF3\uDDFA])|\uD83C\uDDF3(?:\uD83C[\uDDE6\uDDE8\uDDEA-\uDDEC\uDDEE\uDDF1\uDDF4\uDDF5\uDDF7\uDDFA\uDDFF])|\uD83C\uDDE8(?:\uD83C[\uDDE6\uDDE8\uDDE9\uDDEB-\uDDEE\uDDF0-\uDDF5\uDDF7\uDDFA-\uDDFF])|\uD83C\uDDF1(?:\uD83C[\uDDE6-\uDDE8\uDDEE\uDDF0\uDDF7-\uDDFB\uDDFE])|\uD83C\uDDFF(?:\uD83C[\uDDE6\uDDF2\uDDFC])|\uD83C\uDDFC(?:\uD83C[\uDDEB\uDDF8])|\uD83C\uDDFA(?:\uD83C[\uDDE6\uDDEC\uDDF2\uDDF3\uDDF8\uDDFE\uDDFF])|\uD83C\uDDEE(?:\uD83C[\uDDE8-\uDDEA\uDDF1-\uDDF4\uDDF6-\uDDF9])|\uD83C\uDDEF(?:\uD83C[\uDDEA\uDDF2\uDDF4\uDDF5])|(?:\uD83C[\uDFC3\uDFC4\uDFCA]|\uD83D[\uDC6E\uDC71\uDC73\uDC77\uDC81\uDC82\uDC86\uDC87\uDE45-\uDE47\uDE4B\uDE4D\uDE4E\uDEA3\uDEB4-\uDEB6]|\uD83E[\uDD26\uDD37-\uDD39\uDD3D\uDD3E\uDDB8\uDDB9\uDDCD-\uDDCF\uDDD6-\uDDDD])(?:\uD83C[\uDFFB-\uDFFF])|(?:\u26F9|\uD83C[\uDFCB\uDFCC]|\uD83D\uDD75)(?:\uD83C[\uDFFB-\uDFFF])|(?:[\u261D\u270A-\u270D]|\uD83C[\uDF85\uDFC2\uDFC7]|\uD83D[\uDC42\uDC43\uDC46-\uDC50\uDC66\uDC67\uDC6B-\uDC6D\uDC70\uDC72\uDC74-\uDC76\uDC78\uDC7C\uDC83\uDC85\uDCAA\uDD74\uDD7A\uDD90\uDD95\uDD96\uDE4C\uDE4F\uDEC0\uDECC]|\uD83E[\uDD0F\uDD18-\uDD1C\uDD1E\uDD1F\uDD30-\uDD36\uDDB5\uDDB6\uDDBB\uDDD2-\uDDD5])(?:\uD83C[\uDFFB-\uDFFF])|(?:[\u231A\u231B\u23E9-\u23EC\u23F0\u23F3\u25FD\u25FE\u2614\u2615\u2648-\u2653\u267F\u2693\u26A1\u26AA\u26AB\u26BD\u26BE\u26C4\u26C5\u26CE\u26D4\u26EA\u26F2\u26F3\u26F5\u26FA\u26FD\u2705\u270A\u270B\u2728\u274C\u274E\u2753-\u2755\u2757\u2795-\u2797\u27B0\u27BF\u2B1B\u2B1C\u2B50\u2B55]|\uD83C[\uDC04\uDCCF\uDD8E\uDD91-\uDD9A\uDDE6-\uDDFF\uDE01\uDE1A\uDE2F\uDE32-\uDE36\uDE38-\uDE3A\uDE50\uDE51\uDF00-\uDF20\uDF2D-\uDF35\uDF37-\uDF7C\uDF7E-\uDF93\uDFA0-\uDFCA\uDFCF-\uDFD3\uDFE0-\uDFF0\uDFF4\uDFF8-\uDFFF]|\uD83D[\uDC00-\uDC3E\uDC40\uDC42-\uDCFC\uDCFF-\uDD3D\uDD4B-\uDD4E\uDD50-\uDD67\uDD7A\uDD95\uDD96\uDDA4\uDDFB-\uDE4F\uDE80-\uDEC5\uDECC\uDED0-\uDED2\uDED5\uDEEB\uDEEC\uDEF4-\uDEFA\uDFE0-\uDFEB]|\uD83E[\uDD0D-\uDD3A\uDD3C-\uDD45\uDD47-\uDD71\uDD73-\uDD76\uDD7A-\uDDA2\uDDA5-\uDDAA\uDDAE-\uDDCA\uDDCD-\uDDFF\uDE70-\uDE73\uDE78-\uDE7A\uDE80-\uDE82\uDE90-\uDE95])|(?:[#\*0-9\xA9\xAE\u203C\u2049\u2122\u2139\u2194-\u2199\u21A9\u21AA\u231A\u231B\u2328\u23CF\u23E9-\u23F3\u23F8-\u23FA\u24C2\u25AA\u25AB\u25B6\u25C0\u25FB-\u25FE\u2600-\u2604\u260E\u2611\u2614\u2615\u2618\u261D\u2620\u2622\u2623\u2626\u262A\u262E\u262F\u2638-\u263A\u2640\u2642\u2648-\u2653\u265F\u2660\u2663\u2665\u2666\u2668\u267B\u267E\u267F\u2692-\u2697\u2699\u269B\u269C\u26A0\u26A1\u26AA\u26AB\u26B0\u26B1\u26BD\u26BE\u26C4\u26C5\u26C8\u26CE\u26CF\u26D1\u26D3\u26D4\u26E9\u26EA\u26F0-\u26F5\u26F7-\u26FA\u26FD\u2702\u2705\u2708-\u270D\u270F\u2712\u2714\u2716\u271D\u2721\u2728\u2733\u2734\u2744\u2747\u274C\u274E\u2753-\u2755\u2757\u2763\u2764\u2795-\u2797\u27A1\u27B0\u27BF\u2934\u2935\u2B05-\u2B07\u2B1B\u2B1C\u2B50\u2B55\u3030\u303D\u3297\u3299]|\uD83C[\uDC04\uDCCF\uDD70\uDD71\uDD7E\uDD7F\uDD8E\uDD91-\uDD9A\uDDE6-\uDDFF\uDE01\uDE02\uDE1A\uDE2F\uDE32-\uDE3A\uDE50\uDE51\uDF00-\uDF21\uDF24-\uDF93\uDF96\uDF97\uDF99-\uDF9B\uDF9E-\uDFF0\uDFF3-\uDFF5\uDFF7-\uDFFF]|\uD83D[\uDC00-\uDCFD\uDCFF-\uDD3D\uDD49-\uDD4E\uDD50-\uDD67\uDD6F\uDD70\uDD73-\uDD7A\uDD87\uDD8A-\uDD8D\uDD90\uDD95\uDD96\uDDA4\uDDA5\uDDA8\uDDB1\uDDB2\uDDBC\uDDC2-\uDDC4\uDDD1-\uDDD3\uDDDC-\uDDDE\uDDE1\uDDE3\uDDE8\uDDEF\uDDF3\uDDFA-\uDE4F\uDE80-\uDEC5\uDECB-\uDED2\uDED5\uDEE0-\uDEE5\uDEE9\uDEEB\uDEEC\uDEF0\uDEF3-\uDEFA\uDFE0-\uDFEB]|\uD83E[\uDD0D-\uDD3A\uDD3C-\uDD45\uDD47-\uDD71\uDD73-\uDD76\uDD7A-\uDDA2\uDDA5-\uDDAA\uDDAE-\uDDCA\uDDCD-\uDDFF\uDE70-\uDE73\uDE78-\uDE7A\uDE80-\uDE82\uDE90-\uDE95])\uFE0F|(?:[\u261D\u26F9\u270A-\u270D]|\uD83C[\uDF85\uDFC2-\uDFC4\uDFC7\uDFCA-\uDFCC]|\uD83D[\uDC42\uDC43\uDC46-\uDC50\uDC66-\uDC78\uDC7C\uDC81-\uDC83\uDC85-\uDC87\uDC8F\uDC91\uDCAA\uDD74\uDD75\uDD7A\uDD90\uDD95\uDD96\uDE45-\uDE47\uDE4B-\uDE4F\uDEA3\uDEB4-\uDEB6\uDEC0\uDECC]|\uD83E[\uDD0F\uDD18-\uDD1F\uDD26\uDD30-\uDD39\uDD3C-\uDD3E\uDDB5\uDDB6\uDDB8\uDDB9\uDDBB\uDDCD-\uDDCF\uDDD1-\uDDDD])/g;
    };
  }
});

// node_modules/string-width/index.js
var require_string_width = __commonJS({
  "node_modules/string-width/index.js"(exports, module) {
    "use strict";
    var stripAnsi = require_strip_ansi();
    var isFullwidthCodePoint = require_is_fullwidth_code_point();
    var emojiRegex = require_emoji_regex();
    var stringWidth3 = (string) => {
      if (typeof string !== "string" || string.length === 0) {
        return 0;
      }
      string = stripAnsi(string);
      if (string.length === 0) {
        return 0;
      }
      string = string.replace(emojiRegex(), "  ");
      let width2 = 0;
      for (let i = 0; i < string.length; i++) {
        const code = string.codePointAt(i);
        if (code <= 31 || code >= 127 && code <= 159) {
          continue;
        }
        if (code >= 768 && code <= 879) {
          continue;
        }
        if (code > 65535) {
          i++;
        }
        width2 += isFullwidthCodePoint(code) ? 2 : 1;
      }
      return width2;
    };
    module.exports = stringWidth3;
    module.exports.default = stringWidth3;
  }
});

// src/keymap.ts
function parseInput(data) {
  return Array.from(splitInput(data));
}
function* splitInput(data) {
  let text = [];
  const it = data[Symbol.iterator]();
  for (let next = it.next(); !next.done; next = it.next()) {
    const c = next.value;
    if (c.length > 1) {
      text.push(c);
      continue;
    }
    const val = c.charCodeAt(0);
    if (text.length > 0 && (val < 32 || val === 127)) {
      yield {
        inputType: 0 /* Text */,
        data: text
      };
      text = [];
    }
    if (val === 27) {
      const seq2 = it.next();
      if (seq2.done) {
        text.push("\x1B");
        continue;
      }
      let inputType = 22 /* UnsupportedEscape */;
      if (seq2.value !== "[") {
        switch (seq2.value) {
          case "\r":
            inputType = 1 /* AltEnter */;
            break;
        }
        yield {
          inputType,
          data: ["\x1B", seq2.value]
        };
        continue;
      }
      const seq3 = it.next();
      if (seq3.done) {
        continue;
      }
      if (seq3.value >= "0" && seq3.value <= "9") {
        let digit = seq3.value;
        const nextDigit = it.next();
        if (nextDigit.done) {
          return;
        }
        if (nextDigit.value >= "0" && nextDigit.value <= "9") {
          digit += nextDigit.value;
        } else if (nextDigit.value !== "~") {
          continue;
        }
        switch (digit) {
          case "3":
            inputType = 6 /* Delete */;
            break;
        }
        yield {
          inputType,
          data: ["\x1B", "[", digit, "~"]
        };
        continue;
      }
      switch (seq3.value) {
        case "A":
          inputType = 2 /* ArrowUp */;
          break;
        case "B":
          inputType = 3 /* ArrowDown */;
          break;
        case "C":
          inputType = 5 /* ArrowRight */;
          break;
        case "D":
          inputType = 4 /* ArrowLeft */;
          break;
        case "F":
          inputType = 17 /* End */;
          break;
        case "H":
          inputType = 19 /* Home */;
          break;
        case "\r":
          inputType = 1 /* AltEnter */;
          break;
      }
      yield {
        inputType,
        data: ["\x1B", "[", seq3.value]
      };
      continue;
    }
    if (val < 32 || val === 127) {
      let inputType = 21 /* UnsupportedControlChar */;
      switch (val) {
        case 1:
          inputType = 8 /* CtrlA */;
          break;
        case 3:
          inputType = 9 /* CtrlC */;
          break;
        case 4:
          inputType = 10 /* CtrlD */;
          break;
        case 5:
          inputType = 11 /* CtrlE */;
          break;
        case 11:
          inputType = 12 /* CtrlK */;
          break;
        case 17:
          inputType = 14 /* CtrlQ */;
          break;
        case 19:
          inputType = 15 /* CtrlS */;
          break;
        case 21:
          inputType = 16 /* CtrlU */;
          break;
        case 13:
          inputType = 18 /* Enter */;
          break;
        case 127:
          inputType = 7 /* Backspace */;
          break;
        case 12:
          inputType = 13 /* CtrlL */;
          break;
      }
      yield {
        inputType,
        data: [c]
      };
      continue;
    }
    text.push(c);
  }
  if (text.length > 0) {
    yield {
      inputType: 0 /* Text */,
      data: text
    };
  }
}

// src/line.ts
var LineBuffer = class {
  constructor() {
    this.buf = "";
    this.pos = 0;
  }
  buffer() {
    return this.buf;
  }
  pos_buffer() {
    return this.buf.slice(0, this.pos);
  }
  // Return length of buffer in bytes
  length() {
    return this.buf.length;
  }
  // Return length of buffer in characters
  char_length() {
    return [...this.buf].length;
  }
  // Set text and position
  update(text, pos) {
    this.buf = text;
    this.pos = pos;
  }
  insert(text) {
    const shift = text.length;
    const push = this.pos === this.buf.length;
    if (push) {
      this.buf = this.buf + text;
    } else {
      this.buf = this.buf.slice(0, this.pos) + text + this.buf.slice(this.pos);
    }
    this.pos += shift;
    return push;
  }
  moveBack(n) {
    const pos = this.prevPos(n);
    if (pos !== void 0) {
      this.pos = pos;
      return true;
    } else {
      return false;
    }
  }
  moveForward(n) {
    const pos = this.nextPos(n);
    if (pos !== void 0) {
      this.pos = pos;
      return true;
    } else {
      return false;
    }
  }
  moveHome() {
    const start = this.startOfLine();
    if (this.pos > start) {
      this.pos = start;
      return true;
    }
    return false;
  }
  moveEnd() {
    const end = this.endOfLine();
    if (this.pos === end) {
      return false;
    }
    this.pos = end;
    return true;
  }
  startOfLine() {
    const start = this.buf.slice(0, this.pos).lastIndexOf("\n");
    if (start !== -1) {
      return start + 1;
    } else {
      return 0;
    }
  }
  endOfLine() {
    const end = this.buf.slice(this.pos).indexOf("\n");
    if (end !== -1) {
      return this.pos + end;
    } else {
      return this.buf.length;
    }
  }
  moveLineUp(n, promptCols = 0) {
    const off = this.buf.slice(0, this.pos).lastIndexOf("\n");
    if (off === -1) {
      return false;
    }
    const visualCol = [...this.buf.slice(off + 1, this.pos)].length;
    let destStart = this.buf.slice(0, off).lastIndexOf("\n");
    let destIsLine0 = destStart === -1;
    if (destIsLine0) {
      destStart = 0;
    } else {
      destStart = destStart + 1;
    }
    let destEnd = off;
    for (let i = 1; i < n; i++) {
      if (destStart === 0) {
        break;
      }
      destEnd = destStart - 1;
      destStart = this.buf.slice(0, destEnd).lastIndexOf("\n");
      destIsLine0 = destStart === -1;
      if (destIsLine0) {
        destStart = 0;
      } else {
        destStart = destStart + 1;
      }
    }
    const destBufCol = destIsLine0 ? Math.max(0, visualCol - promptCols) : visualCol;
    const slice = [...this.buf.slice(destStart, destEnd)].slice(0, destBufCol);
    let gIdx = off;
    if (slice.length > 0) {
      gIdx = slice.map((c) => c.length).reduce((acc, m) => acc + m, 0);
      gIdx = destStart + gIdx;
    }
    this.pos = gIdx;
    return true;
  }
  moveLineDown(n, promptCols = 0) {
    const off = this.buf.slice(this.pos).indexOf("\n");
    if (off === -1) {
      return false;
    }
    let lineStart = this.buf.slice(0, this.pos).lastIndexOf("\n");
    const currentIsLine0 = lineStart === -1;
    if (currentIsLine0) {
      lineStart = 0;
    } else {
      lineStart += 1;
    }
    const column = [...this.buf.slice(lineStart, this.pos)].length;
    const visualCol = currentIsLine0 ? column + promptCols : column;
    let destStart = this.pos + off + 1;
    let destEnd = this.buf.slice(destStart).indexOf("\n");
    if (destEnd === -1) {
      destEnd = this.buf.length;
    } else {
      destEnd = destStart + destEnd;
    }
    for (let i = 1; i < n; i++) {
      if (destEnd === this.buf.length) {
        break;
      }
      destStart = destEnd + 1;
      destEnd = this.buf.slice(destStart).indexOf("\n");
      if (destEnd === -1) {
        destEnd = this.buf.length;
      } else {
        destEnd = destStart + destEnd;
      }
    }
    const slice = [...this.buf.slice(destStart, destEnd)];
    if (visualCol < slice.length) {
      this.pos = slice.slice(0, visualCol).map((c) => c.length).reduce((acc, m) => acc + m, 0) + destStart;
    } else {
      this.pos = destEnd;
    }
    return true;
  }
  // Set position of cursor
  set_pos(pos) {
    this.pos = pos;
  }
  // Return the position of the character preceding
  // pos
  prevPos(n) {
    if (this.pos === 0) {
      return void 0;
    }
    const buf = this.buf.slice(0, this.pos);
    return this.pos - [...buf].slice(-n).map((c) => c.length).reduce((acc, m) => acc + m, 0);
  }
  // Return the position of the character following the
  // current pos
  nextPos(n) {
    if (this.pos === this.buf.length) {
      return void 0;
    }
    const buf = this.buf.slice(this.pos);
    return this.pos + [...buf].slice(0, n).map((c) => c.length).reduce((acc, m) => acc + m, 0);
  }
  backspace(n) {
    const newPos = this.prevPos(n);
    if (newPos === void 0) {
      return false;
    }
    this.buf = this.buf.slice(0, newPos) + this.buf.slice(this.pos);
    this.pos = newPos;
    return true;
  }
  delete(n) {
    const nextChar = this.nextPos(n);
    if (nextChar !== void 0) {
      this.buf = this.buf.slice(0, this.pos) + this.buf.slice(nextChar);
      return true;
    } else {
      return false;
    }
  }
  deleteEndOfLine() {
    if (this.buf.length == 0 || this.pos == this.buf.length) {
      return false;
    }
    const start = this.pos;
    const end = this.endOfLine();
    if (start == end) {
      this.delete(1);
    } else {
      this.buf = this.buf.slice(0, start) + this.buf.slice(end);
    }
    return true;
  }
};

// src/state.ts
var import_string_width = __toESM(require_string_width());

// src/highlight.ts
var IdentityHighlighter = class {
  highlight(line, pos) {
    return line;
  }
  highlightPrompt(prompt) {
    return prompt;
  }
  highlightChar(line, pos) {
    return false;
  }
};

// src/state.ts
var Position = class {
  constructor(rows, cols) {
    if (rows !== void 0) {
      this.row = rows;
    } else {
      this.row = 0;
    }
    if (cols !== void 0) {
      this.col = cols;
    } else {
      this.col = 0;
    }
  }
};
var Layout = class {
  constructor(promptSize) {
    this.promptSize = promptSize;
    this.cursor = new Position();
    this.end = new Position();
    this.scrollOffset = 0;
  }
};
var State = class {
  constructor(prompt, tty, highlighter, history, restorePrompt) {
    this.restorePrompt = restorePrompt;
    this.line = new LineBuffer();
    this.highlighting = false;
    // Bash-style edit-mode lockout: any successful cursor movement or edit
    // sets editing=true; a buffer replacement via update() (history nav or
    // Ctrl-U) resets it. Up/Down only navigate history while editing=false;
    // once editing, Up/Down move within the buffer and no-op at boundaries.
    this.editing = false;
    this.prompt = restorePrompt ? "" : prompt;
    this.tty = tty;
    this.highlighter = restorePrompt ? {
      highlight: (line, pos) => highlighter.highlight(line, pos),
      highlightChar: (line, pos) => highlighter.highlightChar(line, pos),
      highlightPrompt: () => ""
    } : highlighter;
    this.history = history;
    this.promptSize = tty.calculatePosition(
      this.prompt,
      new Position(0, tty.anchorCol)
    );
    this.layout = new Layout(this.promptSize);
  }
  buffer() {
    return this.line.buffer();
  }
  getTty() {
    return this.tty;
  }
  // Rebase the existing input after the prompt has been printed again.
  reanchor(tty) {
    this.tty = tty;
    this.layout = new Layout(new Position(0, tty.anchorCol));
    this.refresh();
  }
  shouldHighlight() {
    const highlighting = this.highlighter.highlightChar(
      this.line.buf,
      this.line.pos
    );
    if (highlighting) {
      this.highlighting = true;
      return true;
    } else if (this.highlighting) {
      this.highlighting = false;
      return true;
    } else {
      return false;
    }
  }
  clearScreen() {
    this.tty.clearScreen();
    this.tty.anchorRow = 0;
    this.tty.anchorCol = 0;
    this.layout.cursor = new Position();
    this.layout.end = new Position();
    this.layout.scrollOffset = 0;
    this.refresh();
  }
  editInsert(text) {
    this.editing = true;
    const push = this.line.insert(text);
    const multiline = text.includes("\n");
    if (push && !multiline) {
      const width2 = (0, import_string_width.default)(text);
      if (width2 > 0 && this.layout.cursor.col + width2 < this.tty.col && !this.shouldHighlight()) {
        this.layout.cursor.col += width2;
        this.layout.end.col += width2;
        this.tty.write(text);
      } else {
        this.refresh();
      }
    } else {
      this.refresh();
    }
  }
  update(text) {
    this.line.update(text, text.length);
    this.editing = false;
    this.refresh();
  }
  editBackspace(n) {
    if (this.line.backspace(n)) {
      this.editing = true;
      this.refresh();
    }
  }
  editDelete(n) {
    if (this.line.delete(n)) {
      this.editing = true;
      this.refresh();
    }
  }
  editDeleteEndOfLine() {
    if (this.line.deleteEndOfLine()) {
      this.editing = true;
      this.refresh();
    }
  }
  refresh() {
    this.promptSize = this.tty.calculatePosition(
      this.prompt,
      new Position(0, this.tty.anchorCol)
    );
    const newLayout = this.tty.computeLayout(this.promptSize, this.line);
    newLayout.scrollOffset = this.adjustScroll(
      newLayout.cursor.row,
      this.layout.scrollOffset
    );
    if (this.restorePrompt && this.layout.scrollOffset > 0 && (newLayout.scrollOffset === 0 || newLayout.end.row < this.tty.viewportRows()) && this.restorePrompt()) {
      return;
    }
    this.tty.refreshLine(
      this.prompt,
      this.line,
      this.layout,
      newLayout,
      this.highlighter
    );
    this.layout = newLayout;
  }
  // Re-render the current line with no highlighter applied. Intended
  // for commit-time redraws (e.g. on Enter) so the line that ends up
  // in scrollback doesn't have any cursor-driven highlight (e.g. a
  // matching-bracket SGR) baked into it.
  refreshUnhighlighted() {
    const prev = this.highlighter;
    this.highlighter = new IdentityHighlighter();
    try {
      this.refresh();
    } finally {
      this.highlighter = prev;
    }
  }
  adjustScroll(cursorRow, prevOffset) {
    const viewport = this.tty.viewportRows();
    if (cursorRow < prevOffset) {
      return cursorRow;
    }
    if (cursorRow >= prevOffset + viewport) {
      return cursorRow - viewport + 1;
    }
    return prevOffset;
  }
  moveCursorBack(n) {
    if (this.line.moveBack(n)) {
      this.editing = true;
      this.moveCursor();
    }
  }
  moveCursorForward(n) {
    if (this.line.moveForward(n)) {
      this.editing = true;
      this.moveCursor();
    }
  }
  moveCursorUp(n) {
    if (this.editing) {
      if (this.line.moveLineUp(n, this.promptSize.col)) {
        this.moveCursor();
      }
      return;
    }
    this.previousHistory();
  }
  moveCursorDown(n) {
    if (this.editing) {
      if (this.line.moveLineDown(n, this.promptSize.col)) {
        this.moveCursor();
      }
      return;
    }
    this.nextHistory();
  }
  moveCursorHome() {
    if (this.line.moveHome()) {
      this.editing = true;
      this.moveCursor();
    }
  }
  moveCursorEnd() {
    if (this.line.moveEnd()) {
      this.editing = true;
      this.moveCursor();
    }
  }
  moveCursorToEnd() {
    if (this.line.pos === this.line.buf.length) {
      return;
    }
    this.line.pos = this.line.buf.length;
    this.refresh();
  }
  previousHistory() {
    if (this.history.cursor === -1 && this.line.length() > 0) {
      return;
    }
    const prev = this.history.prev();
    if (prev !== void 0) {
      this.update(prev);
    }
  }
  nextHistory() {
    if (this.history.cursor === -1) {
      return;
    }
    const next = this.history.next();
    if (next !== void 0) {
      this.update(next);
    } else {
      this.update("");
    }
  }
  moveCursor() {
    const cursor = this.tty.calculatePosition(
      this.line.pos_buffer(),
      this.promptSize
    );
    if (cursor === this.layout.cursor) {
      return;
    }
    const viewport = this.tty.viewportRows();
    const inWindow = cursor.row >= this.layout.scrollOffset && cursor.row < this.layout.scrollOffset + viewport;
    if (this.shouldHighlight() || !inWindow) {
      this.refresh();
    } else {
      this.tty.moveCursor(this.layout.cursor, cursor);
      this.layout.promptSize = { ...this.promptSize };
      this.layout.cursor = { ...cursor };
    }
  }
};

// src/history.ts
var History = class {
  constructor(maxEntries) {
    this.entries = [];
    this.cursor = -1;
    this.maxEntries = maxEntries;
  }
  saveToLocalStorage() {
    const localStorage = window?.localStorage;
    if (localStorage !== void 0) {
      localStorage.setItem("history", JSON.stringify(this.entries));
    }
  }
  restoreFromLocalStorage() {
    const localStorage = window?.localStorage;
    if (localStorage !== void 0) {
      const historyJson = localStorage.getItem("history");
      if (historyJson === void 0 || historyJson === null) {
        return;
      }
      try {
        const historyEntries = JSON.parse(historyJson);
        if (!Array.isArray(historyEntries) || historyEntries.find((it) => typeof it !== "string") !== void 0) {
          this.entries = [];
          localStorage.setItem("history", "[]");
        } else {
          this.entries = historyEntries;
        }
      } catch (e) {
        this.entries = [];
        localStorage.setItem("history", "[]");
      }
    }
  }
  append(text) {
    this.resetCursor();
    if (!this.entries.includes(text)) {
      this.entries.unshift(text);
    } else {
      this.entries.splice(this.entries.indexOf(text), 1);
      this.entries.unshift(text);
    }
    if (this.entries.length > this.maxEntries) {
      this.entries.pop();
    }
    this.saveToLocalStorage();
  }
  resetCursor() {
    this.cursor = -1;
  }
  next() {
    if (this.cursor === -1) {
      return void 0;
    } else {
      this.cursor -= 1;
    }
    return this.entries[this.cursor];
  }
  prev() {
    if (this.cursor + 1 >= this.entries.length) {
      return void 0;
    } else {
      this.cursor += 1;
    }
    return this.entries[this.cursor];
  }
};

// src/tty.ts
var import_string_width2 = __toESM(require_string_width());
var Tty = class {
  constructor(col, row, tabWidth, out, anchorRow = 0, anchorCol = 0) {
    this.tabWidth = tabWidth;
    this.col = col;
    this.row = row;
    this.anchorRow = anchorRow;
    this.anchorCol = anchorCol;
    this.out = out;
  }
  write(text) {
    return this.out.write(text);
  }
  print(text) {
    return this.out.print(text);
  }
  println(text) {
    return this.out.println(text);
  }
  clearScreen() {
    this.out.write("\x1B[H\x1B[2J");
  }
  viewportRows() {
    return Math.max(this.row - this.anchorRow, 1);
  }
  // Calculate the number of colums and rows required to print
  // text on a this.cols wide terminal starting at orig
  calculatePosition(text, orig) {
    const pos = { ...orig };
    let escSeq = 0;
    [...text].forEach((c) => {
      if (c === "\n") {
        pos.row += 1;
        pos.col = 0;
        return;
      }
      let cw = 0;
      if (c === "	") {
        cw = this.tabWidth - pos.col % this.tabWidth;
      } else {
        let size;
        [size, escSeq] = width(c, escSeq);
        cw = size;
      }
      pos.col += cw;
      if (pos.col > this.col) {
        pos.row += 1;
        pos.col = cw;
      }
    });
    if (pos.col === this.col) {
      pos.col = 0;
      pos.row += 1;
    }
    return pos;
  }
  computeLayout(promptSize, line) {
    const newPromptSize = { ...promptSize };
    const pos = line.pos;
    const cursor = this.calculatePosition(
      line.buf.slice(0, line.pos),
      promptSize
    );
    const end = pos === line.buf.length ? { ...cursor } : this.calculatePosition(line.buf.slice(pos), cursor);
    const newLayout = {
      promptSize: newPromptSize,
      cursor,
      end,
      scrollOffset: 0
    };
    return newLayout;
  }
  // Split highlighted text into visual rows respecting wrap at this.col,
  // re-applying any active SGR escape sequence at the start of each new row.
  splitIntoVisualRows(text, startCol = 0) {
    const rows = [];
    let currentRow = "";
    let col = startCol;
    let escSeq = 0;
    let activeSgr = "";
    let pendingEsc = "";
    const finishEsc = () => {
      if (pendingEsc.endsWith("m")) {
        if (pendingEsc === "\x1B[0m" || pendingEsc === "\x1B[m" || /^\x1b\[0(?:;0)*m$/.test(pendingEsc)) {
          activeSgr = "";
        } else {
          activeSgr = pendingEsc;
        }
      }
      pendingEsc = "";
    };
    for (const c of [...text]) {
      if (escSeq !== 0) {
        currentRow += c;
        pendingEsc += c;
        const [, next] = width(c, escSeq);
        escSeq = next;
        if (escSeq === 0) finishEsc();
        continue;
      }
      if (c === "\x1B") {
        currentRow += c;
        pendingEsc = c;
        const [, next] = width(c, 0);
        escSeq = next;
        if (escSeq === 0) finishEsc();
        continue;
      }
      if (c === "\n") {
        rows.push(currentRow);
        currentRow = activeSgr;
        col = 0;
        continue;
      }
      let cw = 0;
      if (c === "	") {
        cw = this.tabWidth - col % this.tabWidth;
      } else {
        cw = (0, import_string_width2.default)(c);
      }
      if (col + cw > this.col) {
        rows.push(currentRow);
        currentRow = activeSgr + c;
        col = cw;
      } else {
        currentRow += c;
        col += cw;
      }
    }
    rows.push(currentRow);
    if (col === this.col && col > 0) {
      rows.push(activeSgr);
    }
    return rows;
  }
  // Render the layout in a window of viewportRows() rows starting at anchorRow.
  // Only rows in [scrollOffset, scrollOffset + viewportRows) are emitted.
  refreshLine(prompt, line, oldLayout, newLayout, highlighter) {
    this.write("\x1B[?25l");
    try {
      this.refreshLineInner(prompt, line, oldLayout, newLayout, highlighter);
    } finally {
      this.write("\x1B[?25h");
    }
  }
  refreshLineInner(prompt, line, oldLayout, newLayout, highlighter) {
    const oldScroll = oldLayout.scrollOffset ?? 0;
    const newScroll = newLayout.scrollOffset ?? 0;
    const highlighted = highlighter.highlightPrompt(prompt) + highlighter.highlight(line.buf, line.pos);
    const allRows = this.splitIntoVisualRows(highlighted, this.anchorCol);
    const oldCursorViewportRow = Math.max(oldLayout.cursor.row - oldScroll, 0);
    let physicalRow = this.anchorRow + oldCursorViewportRow;
    const desiredVisible = Math.min(allRows.length, this.row);
    const currentBelowAnchor = this.row - this.anchorRow;
    if (desiredVisible > currentBelowAnchor && this.anchorRow > 0) {
      const scrollUp = Math.min(
        desiredVisible - currentBelowAnchor,
        this.anchorRow
      );
      const downBy = this.row - 1 - physicalRow;
      if (downBy > 0) this.write(`\x1B[${downBy}B`);
      this.write("\n".repeat(scrollUp));
      this.anchorRow -= scrollUp;
      physicalRow = this.row - 1;
    }
    const upToAnchor = physicalRow - this.anchorRow;
    if (upToAnchor > 0) this.write(`\x1B[${upToAnchor}A`);
    const viewport = this.viewportRows();
    let effectiveScroll = newScroll;
    if (newLayout.cursor.row < effectiveScroll) {
      effectiveScroll = newLayout.cursor.row;
    } else if (newLayout.cursor.row >= effectiveScroll + viewport) {
      effectiveScroll = newLayout.cursor.row - viewport + 1;
    }
    if (allRows.length <= viewport) effectiveScroll = 0;
    effectiveScroll = Math.max(
      0,
      Math.min(effectiveScroll, allRows.length - viewport)
    );
    newLayout.scrollOffset = effectiveScroll;
    const start = effectiveScroll;
    const end = Math.min(allRows.length, start + viewport);
    const firstCol = start === 0 ? this.anchorCol : 0;
    this.write(`\r${firstCol > 0 ? `\x1B[${firstCol}C` : ""}\x1B[J`);
    for (let i = start; i < end; i++) {
      if (i > start) this.write("\r\n");
      this.write(allRows[i]);
      this.write("\x1B[0m");
    }
    const cursorViewportRow = newLayout.cursor.row - effectiveScroll;
    const lastWrittenViewportRow = end - 1 - start;
    const upBy = Math.max(lastWrittenViewportRow - cursorViewportRow, 0);
    if (upBy > 0) this.write(`\x1B[${upBy}A`);
    if (newLayout.cursor.col > 0) {
      this.write(`\r\x1B[${newLayout.cursor.col}C`);
    } else {
      this.write("\r");
    }
  }
  moveCursor(oldCursor, newCursor) {
    if (newCursor.row > oldCursor.row) {
      const rowShift = newCursor.row - oldCursor.row;
      if (rowShift === 1) {
        this.write("\x1B[B");
      } else {
        this.write(`\x1B[${rowShift}B`);
      }
    } else if (newCursor.row < oldCursor.row) {
      const rowShift = oldCursor.row - newCursor.row;
      if (rowShift === 1) {
        this.write("\x1B[A");
      } else {
        this.write(`\x1B[${rowShift}A`);
      }
    }
    if (newCursor.col > oldCursor.col) {
      const colShift = newCursor.col - oldCursor.col;
      if (colShift === 1) {
        this.write("\x1B[C");
      } else {
        this.write(`\x1B[${colShift}C`);
      }
    } else if (newCursor.col < oldCursor.col) {
      const colShift = oldCursor.col - newCursor.col;
      if (colShift === 1) {
        this.write("\x1B[D");
      } else {
        this.write(`\x1B[${colShift}D`);
      }
    }
    return;
  }
};
function width(text, escSeq) {
  if (escSeq === 1) {
    if (text === "[") {
      return [0, 2];
    } else {
      return [0, 0];
    }
  } else if (escSeq === 2) {
    if (!(text === ";" || text[0] >= "0" && text[0] <= "9")) {
      return [0, 0];
    }
    return [0, escSeq];
  } else if (text === "\x1B") {
    return [0, 1];
  } else if (text === "\n") {
    return [0, escSeq];
  } else {
    return [(0, import_string_width2.default)(text), escSeq];
  }
}

// src/readline.ts
var Readline = class {
  constructor() {
    this.highlighter = new IdentityHighlighter();
    this.history = new History(50);
    this.printingPrompt = false;
    this.pendingKeys = [];
    this.promptGeneration = 0;
    this.promptStartCol = 0;
    this.promptRows = 0;
    this.resizePending = false;
    this.disposables = [];
    this.watermark = 0;
    this.highWatermark = 1e4;
    this.lowWatermark = 1e3;
    this.highWater = false;
    this.state = new State(
      ">",
      this.tty(),
      this.highlighter,
      this.history
    );
    this.checkHandler = () => true;
    this.ctrlCHandler = () => {
      return;
    };
    this.pauseHandler = (resume) => {
      return;
    };
    this.history.restoreFromLocalStorage();
  }
  /**
   * Activate this addon - this function is called by xterm's
   * loadAddon().
   *
   * @param term - The terminal this readline is attached to.
   */
  activate(term) {
    this.promptGeneration++;
    this.term = term;
    this.printingPrompt = false;
    this.pendingKeys = [];
    this.resizePending = false;
    this.promptMarker?.dispose();
    this.promptMarker = void 0;
    this.disposables.push(this.term.onData(this.readData.bind(this)));
    this.disposables.push(
      this.term.onResize(({ cols, rows }) => {
        const tty = this.state.getTty();
        tty.col = cols;
        tty.row = rows;
        if (tty.anchorRow >= rows) tty.anchorRow = Math.max(0, rows - 1);
        if (this.activeRead !== void 0) {
          if (this.printingPrompt) {
            this.resizePending = true;
          } else {
            this.redrawPrompt();
          }
        }
      })
    );
    this.term.attachCustomKeyEventHandler(this.handleKeyEvent.bind(this));
  }
  /**
   * Dispose
   *
   */
  dispose() {
    this.activeRead?.cleanup?.();
    this.promptGeneration++;
    this.printingPrompt = false;
    this.pendingKeys = [];
    this.term = void 0;
    this.promptMarker?.dispose();
    this.disposables.forEach((d) => d.dispose());
  }
  /**
   * Manually append a line to the top of the readline's history.
   *
   * @param text - The text to append to history.
   */
  appendHistory(text) {
    this.history.append(text);
  }
  /**
   * Set the highlighter handler for this readline. This is used to
   * create custom highlighting functionality (e.g. for syntax highlighting
   * or bracket matching).
   *
   * @param highlighter - A handler to handle all highlight callbacks.
   */
  setHighlighter(highlighter) {
    this.highlighter = highlighter;
  }
  /**
   * Set the check callback. This callback is used by readline to determine if input
   * requires additiona lines when the user presses 'enter'.
   *
   * @param fn - A function (string) -> boolean that should return true if the input
   *             is complete, and false if a line (\n) should be added to the input.
   */
  setCheckHandler(fn) {
    this.checkHandler = fn;
  }
  /**
   * Set the ctrl-c handler, called both during and between reads. The handler
   * can abort a read through its caller-owned AbortController. Otherwise an
   * active read clears its input and reprints the prompt as usual.
   *
   * @param fn - The ctrl-c handler.
   */
  setCtrlCHandler(fn) {
    this.ctrlCHandler = fn;
  }
  /**
   * Set the callback to be called when the user presses ctrl-s/ctrl-q.
   *
   * @param fn - The pause handler
   */
  setPauseHandler(fn) {
    this.pauseHandler = fn;
  }
  /**
   * writeReady() may be used to implement basic output flow control. This function
   * will return false if the writes to the terminal initiated by Readline have
   * reached a highwater mark.
   *
   * @returns true if this terminal is accepting more input.
   */
  writeReady() {
    return !this.highWater;
  }
  /**
   * Write text to the terminal.
   *
   * @param text - The text to write to the terminal.
   */
  write(text) {
    if (text === "\n") {
      text = "\r\n";
    } else {
      text = text.replace(/^\n/, "\r\n");
      text = text.replace(/([^\r])\n/g, "$1\r\n");
    }
    const outputLength = text.length;
    this.watermark += outputLength;
    if (this.watermark > this.highWatermark) {
      this.highWater = true;
    }
    if (this.term) {
      this.term.write(text, () => {
        this.watermark = Math.max(this.watermark - outputLength, 0);
        if (this.highWater && this.watermark < this.lowWatermark) {
          this.highWater = false;
        }
      });
    }
  }
  /**
   * Write text to the terminal.
   *
   * @param text - The text to write to the terminal
   */
  print(text) {
    return this.write(text);
  }
  /**
   * Write text to the terminal and append with "\r\n".
   *
   * @param text - The text to write to the terminal./
   * @returns
   */
  println(text) {
    return this.write(text + "\r\n");
  }
  /**
   * Get the current line.
   *
   * @returns string - The current line
   */
  getLine() {
    return this.state.buffer();
  }
  /**
   * Update the current line.
   *
   * @param text - The text to write to the terminal./
   * @returns
   */
  updateLine(text) {
    return this.state.update(text);
  }
  /**
   * Obtain an output interface to this terminal.
   *
   * @returns Output
   */
  output() {
    return this;
  }
  /**
   * Obtain a tty interface to this terminal.
   *
   * @returns A tty
   */
  tty() {
    if (this.term?.options?.tabStopWidth !== void 0) {
      const anchor = this.term.buffer.active.cursorY;
      return new Tty(
        this.term.cols,
        this.term.rows,
        this.term.options.tabStopWidth,
        this.output(),
        anchor,
        this.term.buffer.active.cursorX
      );
    } else {
      return new Tty(0, 0, 8, this.output());
    }
  }
  /**
   * Display the given prompt and wait for one line of input from the
   * terminal. The returned promise will be executed when a line has been
   * read from the terminal.
   *
   * @param prompt The prompt to use.
   * @param options An optional signal; aborting rejects with signal.reason.
   * @returns A promise to be called when the input has been read.
   */
  read(prompt, options = {}) {
    return new Promise((resolve, reject) => {
      const signal = options.signal;
      if (signal?.aborted) {
        reject(signal.reason);
        return;
      }
      if (this.term === void 0) {
        reject("addon is not active");
        return;
      }
      if (this.activeRead !== void 0) {
        reject(new Error("a read is already active"));
        return;
      }
      const read = { prompt, resolve, reject };
      this.activeRead = read;
      if (signal !== void 0) {
        const abort = () => this.abortRead(read, signal.reason);
        read.cleanup = () => signal.removeEventListener("abort", abort);
        signal.addEventListener("abort", abort, { once: true });
      }
      this.printPrompt(prompt, (tty) => {
        this.state = new State(
          prompt,
          tty,
          this.highlighter,
          this.history,
          () => this.redrawPrompt()
        );
        this.state.refresh();
      });
    });
  }
  abortRead(read, reason) {
    if (this.activeRead !== read) return;
    this.activeRead = void 0;
    read.cleanup?.();
    this.promptGeneration++;
    this.printingPrompt = false;
    this.pendingKeys = [];
    this.resizePending = false;
    this.promptMarker?.dispose();
    this.promptMarker = void 0;
    this.state = new State("", this.tty(), this.highlighter, this.history);
    read.reject(reason);
  }
  // Let xterm lay out the prompt, then anchor the editable input at its end.
  // The prompt itself is no longer part of input redraws.
  printPrompt(prompt, done) {
    const term = this.term;
    if (term === void 0) return;
    const read = this.activeRead;
    const generation = ++this.promptGeneration;
    const isCurrent = () => this.term === term && this.activeRead === read && this.promptGeneration === generation;
    this.printingPrompt = true;
    let startRow = 0;
    const finish = () => {
      if (!isCurrent()) return;
      const buffer = term.buffer.active;
      this.promptRows = (buffer.baseY ?? 0) + buffer.cursorY - startRow;
      done(this.tty());
      if (!isCurrent()) return;
      this.printingPrompt = false;
      if (this.resizePending) {
        this.resizePending = false;
        this.redrawPrompt();
        return;
      }
      const pending = this.pendingKeys;
      this.pendingKeys = [];
      for (const key of pending) {
        if (this.activeRead !== read) break;
        this.readKey(key, false);
      }
    };
    const emit = () => {
      if (!isCurrent()) return;
      const buffer = term.buffer.active;
      startRow = (buffer.baseY ?? 0) + buffer.cursorY;
      this.promptStartCol = buffer.cursorX;
      this.promptMarker?.dispose();
      this.promptMarker = term.registerMarker?.(0);
      const highlighted = this.highlighter.highlightPrompt(prompt);
      if (!isCurrent()) return;
      this.write("\x1B[J" + highlighted + "\x1B[0m");
      term.write("", () => {
        if (!isCurrent()) return;
        if (term.buffer.active.cursorX >= term.cols) {
          term.write("\r\n", finish);
        } else {
          finish();
        }
      });
    };
    term.write("", () => {
      if (!isCurrent()) return;
      if (term.buffer.active.cursorX >= term.cols) {
        term.write("\r\n", emit);
      } else {
        emit();
      }
    });
  }
  // Returns false when there is no active read to redraw a prompt for.
  redrawPrompt() {
    if (this.term === void 0 || this.activeRead === void 0) return false;
    const tty = this.state.getTty();
    const marker = this.promptMarker;
    const row = marker !== void 0 && !marker.isDisposed ? marker.line - (this.term.buffer.active.baseY ?? 0) : tty.anchorRow - this.promptRows;
    const startRow = Math.max(0, Math.min(this.term.rows - 1, row));
    const startCol = Math.min(this.promptStartCol, this.term.cols - 1);
    this.term.write(`\x1B[${startRow + 1};${startCol + 1}H`);
    this.printPrompt(
      this.activeRead.prompt,
      (nextTty) => this.state.reanchor(nextTty)
    );
    return true;
  }
  handleKeyEvent(event) {
    if (event.key === "Enter" && event.shiftKey) {
      if (event.type === "keydown") {
        this.readKey({
          inputType: 20 /* ShiftEnter */,
          data: ["\r"]
        });
      }
      return false;
    }
    return true;
  }
  readData(data) {
    const input = parseInput(data);
    if (input.length > 1 || input[0].inputType === 0 /* Text */ && input[0].data.length > 1) {
      this.readPaste(input);
      return;
    }
    this.readKey(input[0]);
  }
  readPaste(input) {
    const mappedInput = input.map((it) => {
      if (it.inputType === 18 /* Enter */) {
        return { inputType: 0 /* Text */, data: ["\n"] };
      }
      return it;
    });
    for (const it of mappedInput) {
      this.readKey(it);
    }
  }
  readKey(input, notifyCtrlC = true) {
    if (input.inputType === 9 /* CtrlC */ && notifyCtrlC) {
      const read = this.activeRead;
      this.ctrlCHandler();
      if (this.activeRead !== read) return;
    }
    if (this.printingPrompt) {
      this.pendingKeys.push(input);
      return;
    }
    if (this.activeRead === void 0) {
      switch (input.inputType) {
        case 13 /* CtrlL */:
          this.write("\x1B[H\x1B[2J");
          break;
      }
      return;
    }
    switch (input.inputType) {
      case 0 /* Text */:
        this.state.editInsert(input.data.join(""));
        break;
      case 1 /* AltEnter */:
      case 20 /* ShiftEnter */:
        this.state.editInsert("\n");
        break;
      case 18 /* Enter */:
        if (this.checkHandler(this.state.buffer())) {
          this.state.moveCursorToEnd();
          this.state.refreshUnhighlighted();
          this.term?.write("\r\n");
          this.history.append(this.state.buffer());
          this.activeRead?.cleanup?.();
          this.activeRead?.resolve(this.state.buffer());
          this.activeRead = void 0;
        } else {
          this.state.editInsert("\n");
        }
        break;
      case 9 /* CtrlC */: {
        this.state.moveCursorToEnd();
        this.term?.write("^C\r\n");
        const prompt = this.activeRead.prompt;
        this.printPrompt(prompt, (tty) => {
          this.state = new State(
            prompt,
            tty,
            this.highlighter,
            this.history,
            () => this.redrawPrompt()
          );
          this.state.refresh();
        });
        break;
      }
      case 15 /* CtrlS */:
        this.pauseHandler(false);
        break;
      case 16 /* CtrlU */:
        this.state.update("");
        break;
      case 12 /* CtrlK */:
        this.state.editDeleteEndOfLine();
        break;
      case 14 /* CtrlQ */:
        this.pauseHandler(true);
        break;
      case 13 /* CtrlL */:
        this.term?.write("\x1B[H\x1B[2J");
        this.printPrompt(this.activeRead.prompt, (tty) => this.state.reanchor(tty));
        break;
      case 19 /* Home */:
      case 8 /* CtrlA */:
        this.state.moveCursorHome();
        break;
      case 17 /* End */:
      case 11 /* CtrlE */:
        this.state.moveCursorEnd();
        break;
      case 7 /* Backspace */:
        this.state.editBackspace(1);
        break;
      case 6 /* Delete */:
      case 10 /* CtrlD */:
        this.state.editDelete(1);
        break;
      case 4 /* ArrowLeft */:
        this.state.moveCursorBack(1);
        break;
      case 5 /* ArrowRight */:
        this.state.moveCursorForward(1);
        break;
      case 2 /* ArrowUp */:
        this.state.moveCursorUp(1);
        break;
      case 3 /* ArrowDown */:
        this.state.moveCursorDown(1);
        break;
      case 21 /* UnsupportedControlChar */:
      case 22 /* UnsupportedEscape */:
        break;
    }
  }
};
export {
  Readline
};
