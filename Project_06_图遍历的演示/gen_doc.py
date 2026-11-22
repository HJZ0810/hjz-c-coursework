from docx import Document
from docx.shared import RGBColor
from docx.enum.text import WD_PARAGRAPH_ALIGNMENT

BLUE = RGBColor(0, 0, 255)

doc = Document('Project_06_图遍历的演示.docx')

doc.add_page_break()

heading = doc.add_heading('V2.0升级说明', level=1)
for run in heading.runs:
    run.font.color.rgb = BLUE

features = [
    ('UTF-8编码', '全面采用UTF-8编码支持，解决中文及多语言字符显示问题，确保跨平台兼容性。'),
    ('ANSI彩色终端', '集成ANSI彩色终端输出，通过不同颜色区分顶点状态、边遍历过程和错误信息，提升可视化体验。'),
    ('CLI(-h/-t)', '新增命令行接口支持：-h参数显示帮助信息，-t参数运行内置测试用例，提供灵活的运行模式。'),
    ('动态内存(栈/队列/邻接表动态扩容)', '实现栈、队列、邻接表的动态内存管理，支持自动扩容，避免固定大小限制，可处理任意规模的图结构。'),
    ('GraphStatus统一错误码', '引入GraphStatus统一错误码机制，规范化错误处理流程，便于调试和问题定位。'),
    ('GRAPHV2魔数文件持久化', '采用GRAPHV2魔数标识的文件格式进行图数据持久化，支持图结构的保存与加载，保证数据完整性。'),
    ('DFS非递归实现(显式栈)', '深度优先搜索采用非递归实现，使用显式栈数据结构，避免递归深度限制，防止栈溢出。'),
    ('BFS环形队列', '广度优先搜索采用环形队列实现，优化内存使用效率，提升遍历性能。'),
    ('生成树树形打印', '新增生成树树形打印功能，以直观的树形结构展示DFS/BFS生成树，清晰呈现遍历路径。'),
    ('14项内置自测', '提供14项内置自测用例，覆盖各种图结构场景，确保功能正确性和稳定性。')
]

for title, desc in features:
    p = doc.add_paragraph()
    run_title = p.add_run(f'■ {title}：')
    run_title.font.color.rgb = BLUE
    run_title.bold = True
    run_desc = p.add_run(desc)
    run_desc.font.color.rgb = BLUE

doc.save('Project_06_图遍历的演示.docx')
print('文档更新成功！已添加V2.0升级说明章节。')
