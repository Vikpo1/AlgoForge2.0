import json
import re
import sys
from urllib.parse import urljoin


IMPORT_ERROR = None

try:
    import requests
    from bs4 import BeautifulSoup, NavigableString
except Exception as exc:
    IMPORT_ERROR = (
        f"Missing Python crawler dependency: {exc}. "
        "Install with: pip install -r backend/scripts/requirements.txt"
    )
    requests = None
    BeautifulSoup = None
    NavigableString = None

try:
    import cloudscraper
except Exception:
    cloudscraper = None


def decode_hex(value):
    try:
        return bytes.fromhex(value).decode("utf-8")
    except Exception as exc:
        raise ValueError(f"invalid hex URL argument: {exc}")


def detect_platform(url):
    lowered = url.lower()
    if "codeforces.com" in lowered:
        return "Codeforces"
    if "atcoder.jp" in lowered:
        return "AtCoder"
    if "luogu.com.cn" in lowered or "luogu.com" in lowered:
        return "Luogu"
    if "qoj.ac" in lowered or "oj.qiuly.org" in lowered or "jiang.ly" in lowered:
        return "QOJ"
    return "Imported"


def ok(url, title, platform, content, content_format="markdown", tags=None, **extra):
    payload = {
        "success": True,
        "title": title or f"{platform} Problem",
        "platform": platform,
        "content": content or "",
        "content_format": content_format,
        "tags": tags or [platform],
        "url": url,
    }
    payload.update(extra)
    return payload


def fail(url, error, platform=None):
    return {
        "success": False,
        "error": str(error),
        "platform": platform or detect_platform(url),
        "url": url,
    }


def make_headers(url=""):
    referer = "https://www.luogu.com.cn/"
    if "codeforces.com" in url:
        referer = "https://codeforces.com/"
    elif "atcoder.jp" in url:
        referer = "https://atcoder.jp/"
    elif "qoj.ac" in url:
        referer = "https://qoj.ac/"

    return {
        "User-Agent": (
            "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
            "AppleWebKit/537.36 (KHTML, like Gecko) "
            "Chrome/125.0.0.0 Safari/537.36"
        ),
        "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8",
        "Accept-Language": "zh-CN,zh;q=0.9,en;q=0.8",
        "Referer": referer,
    }


def get_html(url, use_cloudscraper=False, timeout=15):
    headers = make_headers(url)
    if use_cloudscraper and cloudscraper is not None:
        scraper = cloudscraper.create_scraper(
            browser={"browser": "chrome", "platform": "windows", "mobile": False}
        )
        response = scraper.get(url, headers=headers, timeout=timeout)
    else:
        response = requests.get(url, headers=headers, timeout=timeout, allow_redirects=True)
    response.raise_for_status()
    return response.content.decode("utf-8", errors="replace")


def normalize_space(text):
    text = (text or "").replace("\xa0", " ")
    text = re.sub(r"[ \t\f\v]+", " ", text)
    text = re.sub(r"\n[ \t]+", "\n", text)
    text = re.sub(r"\n{3,}", "\n\n", text)
    return text.strip()


def text_or_empty(node):
    return normalize_space(node.get_text(" ", strip=True)) if node else ""


def plain_inline(node):
    if node is None:
        return ""
    return normalize_space(node.get_text(" ", strip=True))


def pre_text(pre):
    if not pre:
        return ""
    text = pre.get_text("\n", strip=False).replace("\r\n", "\n").replace("\r", "\n")
    text = re.sub(r"\n{2,}", "\n", text)
    return text.strip("\n")


def tag_to_markdown(node):
    if isinstance(node, NavigableString):
        return str(node)

    if not getattr(node, "name", None):
        return ""

    name = node.name.lower()
    if name in {"script", "style", "svg"}:
        return ""
    if name == "br":
        return "\n"
    if name == "pre":
        return f"\n\n```\n{pre_text(node)}\n```\n\n"
    if name == "code":
        return f"`{node.get_text('', strip=False)}`"
    if name in {"h1", "h2", "h3", "h4"}:
        level = {"h1": "#", "h2": "##", "h3": "###", "h4": "####"}[name]
        return f"\n\n{level} {plain_inline(node)}\n\n"
    if name == "li":
        inner = "".join(tag_to_markdown(child) for child in node.children)
        inner = normalize_space(inner).replace("\n", "\n  ")
        return f"- {inner}\n"
    if name in {"ul", "ol"}:
        inner = "".join(tag_to_markdown(child) for child in node.children)
        return f"\n{inner}\n"
    if name == "img":
        alt = node.get("alt") or "image"
        src = node.get("src") or ""
        return f"![{alt}]({src})" if src else alt
    if name == "a":
        href = node.get("href") or ""
        label = plain_inline(node)
        return f"[{label}]({href})" if href and label else label

    inner = "".join(tag_to_markdown(child) for child in node.children)
    if name in {"p", "div", "section", "article", "table", "thead", "tbody", "tr"}:
        return f"\n\n{inner.strip()}\n\n"
    if name in {"td", "th"}:
        return f"{inner.strip()} "
    return inner


def clean_markdown(markdown):
    markdown = (markdown or "").replace("$$$", "$")
    markdown = re.sub(r"\n{3,}", "\n\n", markdown)
    markdown = re.sub(r"[ \t]+\n", "\n", markdown)
    return markdown.strip()


def html_to_markdown(node):
    if node is None:
        return ""
    for removable in node.find_all(["script", "style"]):
        removable.decompose()
    return clean_markdown("".join(tag_to_markdown(child) for child in node.children))


def markdown_for_node(node):
    return clean_markdown(tag_to_markdown(node))


def section_markdown_after_heading(root, headings):
    wanted = [heading.lower() for heading in headings]
    for heading in root.find_all(re.compile(r"^h[1-6]$")):
        text = plain_inline(heading).lower()
        if not any(text == item or item in text for item in wanted):
            continue

        parts = []
        for sibling in heading.next_siblings:
            sibling_name = (getattr(sibling, "name", "") or "").lower()
            if sibling_name in {"h1", "h2", "h3", "h4", "h5", "h6"}:
                break
            parts.append(tag_to_markdown(sibling))
        return normalize_space("".join(parts))
    return ""


def codeforces_header_property(header, class_name):
    if not header:
        return ""

    source = header.find("div", class_=class_name)
    if not source:
        return ""

    clone = BeautifulSoup(str(source), "html.parser").find("div")
    title_node = clone.find("div", class_="property-title") if clone else None
    title = text_or_empty(title_node)
    if title_node:
        title_node.decompose()

    value = text_or_empty(clone)
    if title and value:
        return f"{title}: {value}"
    return text_or_empty(source)


def atcoder_limit_lines(soup):
    text_sources = []
    title_area = soup.select_one("div#main-div") or soup.select_one("div.container")
    task_statement = soup.find("div", id="task-statement")
    if title_area:
        text_sources.append(title_area)
    if task_statement:
        text_sources.append(task_statement)
    text_sources.append(soup)

    for source in text_sources:
        text = normalize_space(source.get_text("\n", strip=True))
        match = re.search(
            r"Time\s*Limit\s*:\s*([^/\n]+?)\s*(?:/|\n)\s*Memory\s*Limit\s*:\s*([^\n]+)",
            text,
            re.IGNORECASE,
        )
        if match:
            return [
                f"Time Limit: {normalize_space(match.group(1))}",
                f"Memory Limit: {normalize_space(match.group(2))}",
            ]

    return []


def codeforces_section_markdown(block, fallback_title):
    clone = BeautifulSoup(str(block), "html.parser").find()
    title_node = clone.find("div", class_="title") if clone else None
    title = text_or_empty(title_node) or fallback_title
    if title_node:
        title_node.decompose()

    body = html_to_markdown(clone)
    if title:
        body = re.sub(rf"^{re.escape(title)}\s*", "", body).strip()
    return f"## {title}\n\n{body}".strip()


def codeforces_samples_markdown(block):
    section_title = text_or_empty(block.find("div", class_="section-title")) or "Example"
    sample_tests = block.find_all("div", class_="sample-test", recursive=False)
    if not sample_tests:
        sample_tests = [block]

    parts = [f"## {section_title}"]
    for index, sample in enumerate(sample_tests, start=1):
        if len(sample_tests) > 1:
            parts.append(f"### Example {index}")

        for sample_part in sample.find_all(
            "div",
            class_=lambda value: value and ("input" in value.split() or "output" in value.split()),
            recursive=False,
        ):
            title = text_or_empty(sample_part.find("div", class_="title")) or "Sample"
            code = pre_text(sample_part.find("pre"))
            if code:
                parts.append(f"### {title}\n\n```\n{code}\n```")

    return "\n\n".join(parts).strip()


def scrape_codeforces(url):
    html = get_html(url, use_cloudscraper=True)
    if "Just a moment" in html or "cf-mitigated" in html:
        raise RuntimeError("Codeforces page is protected by Cloudflare")

    soup = BeautifulSoup(html, "html.parser")
    statement = soup.find("div", class_="problem-statement")
    if not statement:
        statement = soup.find("div", class_="problemindexholder")
    if not statement:
        raise RuntimeError("Codeforces problem statement was not found")

    title_node = statement.find("div", class_="title") or soup.find("div", class_="title")
    title = text_or_empty(title_node)
    if not title and soup.title:
        title = soup.title.get_text(strip=True).replace(" - Codeforces", "").strip()

    header = statement.find("div", class_="header")
    time_limit = codeforces_header_property(header, "time-limit")
    memory_limit = codeforces_header_property(header, "memory-limit")

    input_block = statement.find("div", class_="input-specification")
    output_block = statement.find("div", class_="output-specification")
    samples = statement.find_all("pre")
    tags = [tag.get_text(strip=True) for tag in soup.find_all("span", class_="tag-box")]

    parts = []
    for child in statement.children:
        if isinstance(child, NavigableString):
            if child.strip():
                parts.append(child.strip())
            continue

        classes = child.get("class") or []
        if "header" in classes:
            continue
        if "input-specification" in classes:
            parts.append(codeforces_section_markdown(child, "Input"))
            continue
        if "output-specification" in classes:
            parts.append(codeforces_section_markdown(child, "Output"))
            continue
        if "sample-tests" in classes:
            parts.append(codeforces_samples_markdown(child))
            continue

        text = markdown_for_node(child)
        if text:
            parts.append(text)

    limits = "\n".join(item for item in [time_limit, memory_limit] if item)
    content = ""
    if limits:
        content += f"ALGOFORGE_CF_LIMITS\n{limits}\nALGOFORGE_CF_BODY\n\n"
    content += "\n\n".join(part for part in parts if part).strip()

    return ok(
        url,
        title or "Codeforces Problem",
        "Codeforces",
        content,
        "markdown",
        tags or ["Codeforces"],
        input_description=text_or_empty(input_block),
        output_description=text_or_empty(output_block),
        sample_input=text_or_empty(samples[0]) if len(samples) >= 2 else "",
        sample_output=text_or_empty(samples[1]) if len(samples) >= 2 else "",
    )


def scrape_atcoder(url):
    html = get_html(url)
    soup = BeautifulSoup(html, "html.parser")
    title = soup.title.get_text(strip=True).replace(" - AtCoder", "").strip() if soup.title else "AtCoder Problem"

    statement = soup.select_one("#task-statement .lang-en")
    if not statement:
        statement = soup.find("span", class_="lang-en")
    if not statement:
        statement = soup.find("div", id="task-statement")
    if not statement:
        raise RuntimeError("AtCoder task statement was not found")

    for block in statement.find_all(["pre", "code"]):
        for var in block.find_all("var"):
            var.unwrap()
    for var in statement.find_all("var"):
        var.insert_before("$")
        var.insert_after("$")
        var.unwrap()

    samples = statement.find_all("pre")
    sample_input = ""
    sample_output = ""
    if len(samples) >= 3:
        sample_input = text_or_empty(samples[1])
        sample_output = text_or_empty(samples[2])
    elif len(samples) >= 2:
        sample_input = text_or_empty(samples[0])
        sample_output = text_or_empty(samples[1])

    content = html_to_markdown(statement)
    limits = "\n".join(atcoder_limit_lines(soup))
    if limits:
        content = f"ALGOFORGE_ATCODER_LIMITS\n{limits}\nALGOFORGE_ATCODER_BODY\n\n{content}"

    return ok(
        url,
        title,
        "AtCoder",
        content,
        "markdown",
        ["AtCoder"],
        input_description=section_markdown_after_heading(statement, ["Input", "入力"]),
        output_description=section_markdown_after_heading(statement, ["Output", "出力"]),
        sample_input=sample_input,
        sample_output=sample_output,
    )


def qoj_placeholder(url, title=None, pdf_url=None, reason=None):
    problem_id = url.rstrip("/").split("/")[-1] or "Problem"
    reason_text = f"\n\n> {reason}" if reason else ""
    markers = f"ALGOFORGE_QOJ_STATEMENT: {url}\n"
    if pdf_url:
        markers += f"ALGOFORGE_QOJ_PDF: {pdf_url}\n"
    content = (
        f"{markers}\n"
        "## QOJ PDF 题面\n\n"
        "QOJ 的题面通常以内嵌 PDF 展示，样例、输入格式和输出格式都在同一份 PDF 中。"
        "若已识别到独立 PDF 链接，可在刷题页预览；提交仍请跳转到 QOJ 原题页面完成。"
        f"{reason_text}"
    )
    return ok(
        url,
        title or f"QOJ {problem_id}",
        "QOJ",
        content,
        "markdown",
        ["QOJ"],
        input_description="QOJ 题面为 PDF，请在原题页面查看输入格式。",
        output_description="QOJ 题面为 PDF，请在原题页面查看输出格式。",
        sample_input="样例在 QOJ PDF 题面中。",
        sample_output="样例在 QOJ PDF 题面中。",
    )


def scrape_qoj(url):
    html = get_html(url, use_cloudscraper=True)
    if "Just a moment" in html or "challenge-platform" in html or "cf-mitigated" in html:
        return qoj_placeholder(url, reason="QOJ 当前触发了 Cloudflare 校验。")

    soup = BeautifulSoup(html, "html.parser")
    title = "QOJ Problem"
    heading = soup.find(["h1", "h2"])
    if heading:
        heading_text = heading.get_text(" ", strip=True)
        if heading_text and "QOJ" not in heading_text:
            title = heading_text.lstrip("# ").strip()
    if soup.title and title == "QOJ Problem":
        title = soup.title.get_text(strip=True).replace(" - QOJ.ac", "").strip()

    pdf_url = ""
    for node in soup.find_all(["iframe", "embed", "object"]):
        candidate = node.get("src") or node.get("data") or ""
        if candidate and ("download.php" in candidate or ".pdf" in candidate.lower() or "statement" in candidate.lower()):
            pdf_url = urljoin(url, candidate)
            break

    if not pdf_url:
        for link in soup.find_all("a"):
            href = link.get("href") or ""
            text = link.get_text(" ", strip=True).lower()
            if href and ("download.php" in href or ".pdf" in href.lower() or "statement" in text or "pdf" in text):
                pdf_url = urljoin(url, href)
                break

    return qoj_placeholder(url, title=title, pdf_url=pdf_url or None)


def luogu_title_from_soup(soup):
    if soup.title:
        title = soup.title.get_text(strip=True)
        title = title.replace(" - 洛谷 | 计算机科学教育新生态", "")
        title = title.replace(" - 洛谷", "")
        return title.strip()
    return "Luogu Problem"


def scrape_luogu(url):
    html = get_html(url, use_cloudscraper=True)
    soup = BeautifulSoup(html, "html.parser")
    title = luogu_title_from_soup(soup)

    script = soup.find("script", id="lentille-context")
    if script and script.string:
        data = json.loads(script.string)["data"]["problem"]
        content = data.get("content") or data.get("contenu") or {}
        pid = data.get("pid") or ""
        name = data.get("name") or content.get("name") or ""
        if pid and name:
            title = f"{pid} {name}"
        elif name:
            title = name

        markdown = ""
        if content.get("background"):
            markdown += f"## 题目背景\n\n{content['background']}\n\n"
        if content.get("description"):
            markdown += f"## 题目描述\n\n{content['description']}\n\n"
        if content.get("hint"):
            markdown += f"## 说明/提示\n\n{content['hint']}\n\n"

        samples = data.get("samples") or []
        sample_input = samples[0][0] if samples and len(samples[0]) >= 2 else ""
        sample_output = samples[0][1] if samples and len(samples[0]) >= 2 else ""

        return ok(
            url,
            title,
            "Luogu",
            markdown.strip(),
            "markdown",
            ["Luogu"],
            input_description=content.get("formatI", ""),
            output_description=content.get("formatO", ""),
            sample_input=sample_input,
            sample_output=sample_output,
        )

    article = soup.find("article")
    if not article:
        raise RuntimeError("Luogu lentille-context and article were not found")

    return ok(url, title, "Luogu", html_to_markdown(article), "markdown", ["Luogu"])


def scrape_problem(url):
    if IMPORT_ERROR:
        return fail(url, IMPORT_ERROR)

    platform = detect_platform(url)
    try:
        if platform == "Codeforces":
            return scrape_codeforces(url)
        if platform == "AtCoder":
            return scrape_atcoder(url)
        if platform == "Luogu":
            return scrape_luogu(url)
        if platform == "QOJ":
            return scrape_qoj(url)
        return fail(url, f"Unsupported platform: {platform}", platform)
    except Exception as exc:
        return fail(url, exc, platform)


def main():
    if len(sys.argv) < 2:
        print(json.dumps(fail("", "No URL hex argument provided"), ensure_ascii=True))
        return 1

    try:
        url = decode_hex(sys.argv[1])
    except Exception as exc:
        print(json.dumps(fail("", exc), ensure_ascii=True))
        return 1

    print(json.dumps(scrape_problem(url), ensure_ascii=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
