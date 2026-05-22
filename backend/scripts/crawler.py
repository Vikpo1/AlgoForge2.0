import sys
import requests
from bs4 import BeautifulSoup
from markdownify import markdownify as md
import urllib.parse

# ==========================================
# 1. 定义爬虫基类 (相当于 C++ 的虚基类)
# ==========================================
class BaseOJCrawler:
    def __init__(self, url):
        self.url = url
        self.headers = {
            "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36"
        }
        self.html_content = ""

    def fetch_html(self):
        """通用的网络请求方法"""
        response = requests.get(self.url, headers=self.headers, timeout=10)
        response.raise_for_status()
        self.html_content = response.text
        return BeautifulSoup(self.html_content, 'html.parser')

    # 以下两个方法留给子类去实现 (相当于 C++ 的纯虚函数 virtual void ... = 0;)
    def extract_title(self, soup):
        raise NotImplementedError

    def extract_markdown(self, soup):
        raise NotImplementedError

    def run(self):
        """执行爬取流水线"""
        try:
            soup = self.fetch_html()
            title = self.extract_title(soup)
            content = self.extract_markdown(soup)
            return {"status": "success", "title": title, "content": content}
        except Exception as e:
            return {"status": "error", "message": str(e)}

# ==========================================
# 2. 具体实现类：洛谷爬虫
# ==========================================
class LuoguCrawler(BaseOJCrawler):
    def extract_title(self, soup):
        # 洛谷的标题通常在 title 标签里，格式如 "P1001 A+B Problem - 洛谷"
        title_tag = soup.find('title')
        return title_tag.text.split('-')[0].strip() if title_tag else "未知洛谷题目"

    def extract_markdown(self, soup):
        # 洛谷的题目主体内容通常包裹在一个特定的 article 标签或 div 里
        # (注：洛谷实际上有很多反爬和动态渲染，这里用最简单的静态抓取做演示)
        article = soup.find('article')
        if not article:
            return "未能找到题目核心区域，可能是因为洛谷开启了动态渲染或反爬。"
        
        # 使用 markdownify 神器，一键把 HTML 转化为 Markdown 格式！
        return md(str(article), heading_style="ATX")

# ==========================================
# 3. 具体实现类：Codeforces 爬虫 (预留扩展)
# ==========================================
class CodeforcesCrawler(BaseOJCrawler):
    def extract_title(self, soup):
        # CF 的标题在 <div class="title"> 里
        title_div = soup.find('div', class_='title')
        return title_div.text.strip() if title_div else "Unknown CF Problem"

    def extract_markdown(self, soup):
        # CF 的题目主体在 <div class="problem-statement"> 里
        statement = soup.find('div', class_='problem-statement')
        if not statement:
            return "Cannot find CF problem statement."
        return md(str(statement), heading_style="ATX")

# ==========================================
# 4. 爬虫工厂 (核心调度器)
# ==========================================
def CrawlerFactory(url):
    """根据 URL 的域名，自动分配对应的爬虫类"""
    domain = urllib.parse.urlparse(url).netloc
    
    if "luogu.com.cn" in domain:
        return LuoguCrawler(url)
    elif "codeforces.com" in domain:
        return CodeforcesCrawler(url)
    else:
        raise ValueError(f"暂不支持抓取该网站: {domain}")

# ==========================================
# 5. 主程序入口
# ==========================================
if __name__ == "__main__":
    target_url = sys.argv[1] if len(sys.argv) > 1 else "https://www.luogu.com.cn/problem/P1001"
    
    print(f"[SYSTEM] 目标 URL: {target_url}")
    
    try:
        # 1. 工厂分配爬虫
        crawler = CrawlerFactory(target_url)
        print(f"[SYSTEM] 已分配爬虫核心: {crawler.__class__.__name__}")
        
        # 2. 执行抓取
        result = crawler.run()
        
        # 3. 输出结果 (后续 C++ 将通过读取 stdout 来获取这些 JSON 数据)
        if result["status"] == "success":
            print(f"\n# {result['title']}\n")
            print(result['content'])
        else:
            print(f"抓取失败: {result['message']}")
            
    except Exception as e:
        print(f"系统级错误: {e}")