// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QByteArray>
#include <QObject>
#include <QTimer>

#include <functional>
#include <utility>

namespace KWallet
{
class Wallet;
}

class KWalletKeyProvider : public QObject
{
    Q_OBJECT

  public:
    enum class State
    {
        Available,
        Absent,
        Locked,
        Cancelled,
        Unavailable,
    };
    Q_ENUM(State)

    struct Result
    {
        State state = State::Unavailable;
        QByteArray key;

        Result() = default;
        Result(State value, QByteArray secret) : state(value), key(std::move(secret)) {}
        Result(const Result &) = delete;
        Result &operator=(const Result &) = delete;
        Result(Result &&other) noexcept : state(other.state), key(std::move(other.key)) {}
        Result &operator=(Result &&other) noexcept
        {
            if (this != &other)
            {
                clear();
                state = other.state;
                key = std::move(other.key);
            }
            return *this;
        }
        ~Result()
        {
            clear();
        }

        void clear()
        {
            key.fill(0);
            key.clear();
        }
    };

    using Completion = std::function<void(Result result)>;

    explicit KWalletKeyProvider(QObject *parent = nullptr);
    ~KWalletKeyProvider() override;

    [[nodiscard]] virtual State boundedState() const;
    virtual void requestKey(Completion completion);
    [[nodiscard]] virtual Result generateTransientKey() const;
    virtual void storeKey(QByteArray key, Completion completion);
    virtual void deleteKey(Completion completion);
    virtual void cancel();

  private:
    void finishOpen(bool success);
    void performRead();
    void finish(Result result);
    [[nodiscard]] bool prepareFolder(bool create);

    enum class PendingAction
    {
        None,
        Read,
        Store,
        Delete,
    };

    KWallet::Wallet *m_wallet = nullptr;
    Completion m_completion;
    PendingAction m_action = PendingAction::None;
    QByteArray m_pendingKey;
    QTimer m_accessTimer;
};
