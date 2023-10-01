#include "CppUnitTest.h"

#include "ui\viewmodels\OverlayListPageViewModel.hh"

#include "tests\mocks\MockImageRepository.hh"
#include "tests\mocks\MockOverlayManager.hh"
#include "tests\RA_UnitTestHelpers.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(OverlayListPageViewModel_Tests)
{
private:
    class OverlayListPageViewModelHarness : public OverlayListPageViewModel
    {
    public:
        ra::ui::mocks::MockImageRepository mockImageRepository;
        ra::ui::viewmodels::mocks::MockOverlayManager mockOverlayManager;

        void Refresh() override
        {
            OverlayListPageViewModel::Refresh();
            m_bHasDetail = true;

            AddItem(1, L"One");
            AddItem(2, L"Two");
            AddItem(3, L"Three");
            AddItem(4, L"Four");
            AddItem(5, L"Five");
            AddItem(6, L"Six");
            AddItem(7, L"Seven");
            AddItem(8, L"Eight");
            AddItem(9, L"Nine");
            AddItem(10, L"Ten");
            AddItem(11, L"Eleven");
            AddItem(12, L"Twelve");

            m_nVisibleItems = 5;
        }

        void RefreshWithHeader()
        {
            OverlayListPageViewModel::Refresh();
            m_bHasDetail = true;

            m_vItems.Clear();
            AddItem(0, L"Header 1");
            AddItem(1, L"One");
            AddItem(2, L"Two");
            AddItem(0, L"Header 2");
            AddItem(3, L"Three");
            AddItem(4, L"Four");
            AddItem(0, L"Header 3");
            AddItem(5, L"Five");
            AddItem(6, L"Six");
            AddItem(7, L"Seven");
            AddItem(8, L"Eight");
            AddItem(9, L"Nine");
            AddItem(10, L"Ten");

            m_nVisibleItems = 5;
            EnsureSelectedItemIndexValid();
        }

        void AddItem(int nId, const std::wstring&& sLabel)
        {
            auto& vmItem = m_vItems.Add();
            vmItem.SetId(nId);
            vmItem.SetLabel(sLabel);            
        }

        void AddImage(gsl::index nIndex, std::string sName)
        {
            m_vItems.GetItemAt(nIndex)->Image.ChangeReference(ra::ui::ImageType::Badge, sName);
        }

        void FetchItemDetail(ItemViewModel& vmItem) noexcept override
        {
            vmDetail = &vmItem;
        }

        gsl::index GetScrollOffset() const noexcept { return m_nScrollOffset; }
        bool IsDetail() const noexcept { return m_bDetail; }
        size_t GetItemCount() const noexcept { return m_vItems.Count(); }

        ItemViewModel* vmDetail = nullptr;
    };

public:
    TEST_METHOD(TestInitialValues)
    {
        OverlayListPageViewModelHarness vmListPage;
        Assert::AreEqual({ 0 }, vmListPage.GetScrollOffset());
        Assert::AreEqual(0, vmListPage.GetSelectedItemIndex());
        Assert::AreEqual({ 0U }, vmListPage.GetItemCount());
        Assert::IsFalse(vmListPage.IsDetail());
        Assert::AreEqual(std::wstring(), vmListPage.GetTitle());
        Assert::AreEqual(std::wstring(), vmListPage.GetSubTitle());
        Assert::AreEqual(std::wstring(), vmListPage.GetTitleDetail());
    }

    TEST_METHOD(TestRefresh)
    {
        OverlayListPageViewModelHarness vmListPage;
        vmListPage.Refresh();

        Assert::AreEqual({ 0 }, vmListPage.GetScrollOffset());
        Assert::AreEqual(0, vmListPage.GetSelectedItemIndex());
        Assert::AreEqual({ 12U }, vmListPage.GetItemCount());
        Assert::IsFalse(vmListPage.IsDetail());
        Assert::AreEqual(std::wstring(), vmListPage.GetTitle());
        Assert::AreEqual(std::wstring(), vmListPage.GetSubTitle());
        Assert::AreEqual(std::wstring(), vmListPage.GetTitleDetail());
    }

    TEST_METHOD(TestUpdateEmpty)
    {
        OverlayListPageViewModelHarness vmListPage;
        Assert::IsFalse(vmListPage.Update(0.1));
    }

    TEST_METHOD(TestUpdateAfterRefresh)
    {
        OverlayListPageViewModelHarness vmListPage;
        vmListPage.Refresh();

        // first update will check the pending images - since none are requested, pending count will drop to 0
        Assert::IsTrue(vmListPage.Update(0.1));

        // now that pending image count is zero, update should return false
        Assert::IsFalse(vmListPage.Update(0.1));
    }

    TEST_METHOD(TestUpdateAfterRefreshPendingImages)
    {
        OverlayListPageViewModelHarness vmListPage;
        vmListPage.Refresh();
        vmListPage.AddImage(0, "One");
        vmListPage.AddImage(1, "Two");

        // first update will check the pending images - since none are requested, pending count will drop to 0
        Assert::IsTrue(vmListPage.Update(0.1));

        // next update will still have two pending images, so returns false
        Assert::IsFalse(vmListPage.Update(0.1));

        // one image becomes available, update should return true
        vmListPage.mockImageRepository.SetImageAvailable(ra::ui::ImageType::Badge, "Two");
        Assert::IsTrue(vmListPage.Update(0.1));

        // next update will still have one pending images, so returns false
        Assert::IsFalse(vmListPage.Update(0.1));

        // last image becomes available, update should return true
        vmListPage.mockImageRepository.SetImageAvailable(ra::ui::ImageType::Badge, "One");
        Assert::IsTrue(vmListPage.Update(0.1));

        // no more pending images
        Assert::IsFalse(vmListPage.Update(0.1));
    }

    TEST_METHOD(TestProcessInputEmpty)
    {
        OverlayListPageViewModelHarness vmListPage;
        ControllerInput pInput{};

        pInput.m_bDownPressed = true;
        Assert::IsFalse(vmListPage.ProcessInput(pInput));
        pInput.m_bDownPressed = false;

        pInput.m_bUpPressed = true;
        Assert::IsFalse(vmListPage.ProcessInput(pInput));
        pInput.m_bUpPressed = false;

        pInput.m_bConfirmPressed = true;
        Assert::IsFalse(vmListPage.ProcessInput(pInput));
        pInput.m_bConfirmPressed = false;

        pInput.m_bCancelPressed = true;
        Assert::IsFalse(vmListPage.ProcessInput(pInput));
        pInput.m_bCancelPressed = false;
    }

    TEST_METHOD(TestProcessInputUpDown)
    {
        OverlayListPageViewModelHarness vmListPage;
        vmListPage.Refresh();

        ControllerInput pInput{};
        pInput.m_bDownPressed = true;
        Assert::IsTrue(vmListPage.ProcessInput(pInput));
        Assert::AreEqual(1, vmListPage.GetSelectedItemIndex());
        Assert::AreEqual({ 0 }, vmListPage.GetScrollOffset());

        Assert::IsTrue(vmListPage.ProcessInput(pInput));
        Assert::AreEqual(2, vmListPage.GetSelectedItemIndex());
        Assert::AreEqual({ 0 }, vmListPage.GetScrollOffset());

        Assert::IsTrue(vmListPage.ProcessInput(pInput));
        Assert::AreEqual(3, vmListPage.GetSelectedItemIndex());
        Assert::AreEqual({ 0 }, vmListPage.GetScrollOffset());

        Assert::IsTrue(vmListPage.ProcessInput(pInput));
        Assert::AreEqual(4, vmListPage.GetSelectedItemIndex());
        Assert::AreEqual({ 0 }, vmListPage.GetScrollOffset());

        Assert::IsTrue(vmListPage.ProcessInput(pInput));
        Assert::AreEqual(5, vmListPage.GetSelectedItemIndex());
        Assert::AreEqual({ 1 }, vmListPage.GetScrollOffset());

        // five visible items, list should start scrolling now
        Assert::IsTrue(vmListPage.ProcessInput(pInput));
        Assert::AreEqual(6, vmListPage.GetSelectedItemIndex());
        Assert::AreEqual({ 2 }, vmListPage.GetScrollOffset());

        Assert::IsTrue(vmListPage.ProcessInput(pInput));
        Assert::AreEqual(7, vmListPage.GetSelectedItemIndex());
        Assert::AreEqual({ 3 }, vmListPage.GetScrollOffset());

        Assert::IsTrue(vmListPage.ProcessInput(pInput));
        Assert::AreEqual(8, vmListPage.GetSelectedItemIndex());
        Assert::AreEqual({ 4 }, vmListPage.GetScrollOffset());

        Assert::IsTrue(vmListPage.ProcessInput(pInput));
        Assert::AreEqual(9, vmListPage.GetSelectedItemIndex());
        Assert::AreEqual({ 5 }, vmListPage.GetScrollOffset());

        Assert::IsTrue(vmListPage.ProcessInput(pInput));
        Assert::AreEqual(10, vmListPage.GetSelectedItemIndex());
        Assert::AreEqual({ 6 }, vmListPage.GetScrollOffset());

        Assert::IsTrue(vmListPage.ProcessInput(pInput));
        Assert::AreEqual(11, vmListPage.GetSelectedItemIndex());
        Assert::AreEqual({ 7 }, vmListPage.GetScrollOffset());

        // end of list
        Assert::IsFalse(vmListPage.ProcessInput(pInput));
        Assert::AreEqual(11, vmListPage.GetSelectedItemIndex());
        Assert::AreEqual({ 7 }, vmListPage.GetScrollOffset());

        // going up shouldn't immediately update the scroll offset
        pInput.m_bDownPressed = false;
        pInput.m_bUpPressed = true;
        Assert::IsTrue(vmListPage.ProcessInput(pInput));
        Assert::AreEqual(10, vmListPage.GetSelectedItemIndex());
        Assert::AreEqual({ 7 }, vmListPage.GetScrollOffset());

        Assert::IsTrue(vmListPage.ProcessInput(pInput));
        Assert::AreEqual(9, vmListPage.GetSelectedItemIndex());
        Assert::AreEqual({ 7 }, vmListPage.GetScrollOffset());

        Assert::IsTrue(vmListPage.ProcessInput(pInput));
        Assert::AreEqual(8, vmListPage.GetSelectedItemIndex());
        Assert::AreEqual({ 7 }, vmListPage.GetScrollOffset());

        Assert::IsTrue(vmListPage.ProcessInput(pInput));
        Assert::AreEqual(7, vmListPage.GetSelectedItemIndex());
        Assert::AreEqual({ 7 }, vmListPage.GetScrollOffset());

        Assert::IsTrue(vmListPage.ProcessInput(pInput));
        Assert::AreEqual(6, vmListPage.GetSelectedItemIndex());
        Assert::AreEqual({ 6 }, vmListPage.GetScrollOffset());

        // now we should start scrolling again
        Assert::IsTrue(vmListPage.ProcessInput(pInput));
        Assert::AreEqual(5, vmListPage.GetSelectedItemIndex());
        Assert::AreEqual({ 5 }, vmListPage.GetScrollOffset());

        Assert::IsTrue(vmListPage.ProcessInput(pInput));
        Assert::AreEqual(4, vmListPage.GetSelectedItemIndex());
        Assert::AreEqual({ 4 }, vmListPage.GetScrollOffset());

        Assert::IsTrue(vmListPage.ProcessInput(pInput));
        Assert::AreEqual(3, vmListPage.GetSelectedItemIndex());
        Assert::AreEqual({ 3 }, vmListPage.GetScrollOffset());

        Assert::IsTrue(vmListPage.ProcessInput(pInput));
        Assert::AreEqual(2, vmListPage.GetSelectedItemIndex());
        Assert::AreEqual({ 2 }, vmListPage.GetScrollOffset());

        Assert::IsTrue(vmListPage.ProcessInput(pInput));
        Assert::AreEqual(1, vmListPage.GetSelectedItemIndex());
        Assert::AreEqual({ 1 }, vmListPage.GetScrollOffset());

        Assert::IsTrue(vmListPage.ProcessInput(pInput));
        Assert::AreEqual(0, vmListPage.GetSelectedItemIndex());
        Assert::AreEqual({ 0 }, vmListPage.GetScrollOffset());

        // end of list
        Assert::IsFalse(vmListPage.ProcessInput(pInput));
        Assert::AreEqual(0, vmListPage.GetSelectedItemIndex());
        Assert::AreEqual({ 0 }, vmListPage.GetScrollOffset());
    }

    TEST_METHOD(TestRefreshFewer)
    {
        OverlayListPageViewModelHarness vmListPage;
        vmListPage.RefreshWithHeader(); // 13 items

        // scroll to bottom
        ControllerInput pInput{};
        pInput.m_bDownPressed = true;
        for (size_t i = 0; i < vmListPage.GetItemCount(); ++i)
            vmListPage.ProcessInput(pInput);

        // add some items
        for (int i = 13; i < 20; ++i)
            vmListPage.AddItem(i, std::to_wstring(i));

        Assert::AreEqual(12, vmListPage.GetSelectedItemIndex());
        Assert::AreEqual({ 8 }, vmListPage.GetScrollOffset());

        // reset to 13 items - cursor position should be remembered
        vmListPage.RefreshWithHeader();
        Assert::AreEqual(12, vmListPage.GetSelectedItemIndex());
        Assert::AreEqual({ 8 }, vmListPage.GetScrollOffset());

        // add some items and scroll down a couple
        for (int i = 13; i < 20; ++i)
            vmListPage.AddItem(i, std::to_wstring(i));
        vmListPage.ProcessInput(pInput);
        vmListPage.ProcessInput(pInput);
        vmListPage.ProcessInput(pInput);

        // reset to 13 items - cursor position is invalid, so it should be reset
        vmListPage.RefreshWithHeader();
        Assert::AreEqual(0, vmListPage.GetSelectedItemIndex());
        Assert::AreEqual({ 0 }, vmListPage.GetScrollOffset());
    }

    TEST_METHOD(TestProcessInputDetailToggle)
    {
        OverlayListPageViewModelHarness vmListPage;
        vmListPage.Refresh();
        Assert::IsNull(vmListPage.vmDetail);

        ControllerInput pInputConfirm{};
        pInputConfirm.m_bConfirmPressed = true;
        Assert::IsTrue(vmListPage.ProcessInput(pInputConfirm));
        Assert::AreEqual(std::wstring(), vmListPage.GetTitle());
        Assert::AreEqual(0, vmListPage.GetSelectedItemIndex());
        Assert::AreEqual(1, vmListPage.vmDetail->GetId());

        // pressing confirm on the detail page does nothing
        Assert::IsTrue(vmListPage.ProcessInput(pInputConfirm));
        Assert::AreEqual(std::wstring(), vmListPage.GetTitle());
        Assert::AreEqual(0, vmListPage.GetSelectedItemIndex());
        Assert::AreEqual(1, vmListPage.vmDetail->GetId());

        // pressing down moves to the next item
        ControllerInput pInputDown{};
        pInputDown.m_bDownPressed = true;
        Assert::IsTrue(vmListPage.ProcessInput(pInputDown));
        Assert::AreEqual(std::wstring(), vmListPage.GetTitle());
        Assert::AreEqual(1, vmListPage.GetSelectedItemIndex());
        Assert::AreEqual(2, vmListPage.vmDetail->GetId());

        Assert::IsTrue(vmListPage.ProcessInput(pInputDown));
        Assert::AreEqual(std::wstring(), vmListPage.GetTitle());
        Assert::AreEqual(2, vmListPage.GetSelectedItemIndex());
        Assert::AreEqual(3, vmListPage.vmDetail->GetId());

        // pressing cancel closes the detail page, but keeps the selection
        vmListPage.vmDetail = nullptr;
        ControllerInput pInputCancel{};
        pInputCancel.m_bCancelPressed = true;
        Assert::IsTrue(vmListPage.ProcessInput(pInputCancel));
        Assert::AreEqual(std::wstring(), vmListPage.GetTitle());
        Assert::AreEqual(2, vmListPage.GetSelectedItemIndex());
        Assert::IsNull(vmListPage.vmDetail);

        // pressing confirm reopens the detail page
        Assert::IsTrue(vmListPage.ProcessInput(pInputConfirm));
        Assert::AreEqual(std::wstring(), vmListPage.GetTitle());
        Assert::AreEqual(2, vmListPage.GetSelectedItemIndex());
        Assert::AreEqual(3, vmListPage.vmDetail->GetId());
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
